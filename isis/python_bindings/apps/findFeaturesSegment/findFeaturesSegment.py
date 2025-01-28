#!/usr/bin/env python

import os
import sys
import re 

import kalasiris as kisis

import subprocess
import pvl
import shutil

from glob import glob
from pathlib import Path
import logging as log
from copy import deepcopy
import tempfile

import plio
from plio.io import io_controlnetwork as cnet
from plio.io import isis_serial_number as sn

from subprocess import check_output
from multiprocessing.pool import ThreadPool
import multiprocessing
import itertools
from functools import reduce
from subprocess import check_output
from itertools import compress
from math import ceil

import astroset

MAX_LEN = 30000

# remove progress bar output from string
filter_progress = lambda x: "\n".join([line for line in x.split("\n") if not "% Processed" in line and not "Working" in line])


def footprintinit(img : str):
    """
    Just a wrapper for footprintinit with logging and exception handling 
    Probably should be a functor in kalasiris
    """
    try:
        ret = kisis.footprintinit(img)
        log.debug(f"returned: {ret}")
        return ret
    except subprocess.CalledProcessError as err:
        log.debug('Had an ISIS error:')
        log.debug(' '.join(err.cmd))
        log.debug(err.stdout)
        log.debug(err.stderr)


def read_cubelist(cube_list : Path):
    """
    read_cubelist accepts a filelist path and returns a list object

    Parameters
    ----------
    cube_list : Path
                path-like object pointing to filelist 

    Returns
    -------
    list
        list of files 
    """
    files = []
    
    with open(cube_list) as c:
        content = c.read()
        content = content.strip()
        files = content.split("\n")
    
    for i in range(len(files)): 
        files[i] = os.path.abspath(files[i])
    return files




def segment(img_path : Path, workdir : Path, nlines : int = MAX_LEN):
    """
    Segments an image into multiple parts.

    Parameters
    ----------
    img_path : path-like
        The path to the image to be segmented.
    nlines : int
        The number of lines in each segment (defaults to MAX_LEN).

    Returns
    -------
    segment_metadata : list of dict
        A list of dictionaries containing metadata for each segment.
    """

    if isinstance(img_path, str):
        img_path = Path(img_path)
    log.debug(f"nlines: {nlines}")

    segment_metadata = {}
    try:
        # change workdir so output goes there 
        oldpwd = os.getcwd()
        
        # create if it doesn't exist
        workdir.mkdir(parents=True, exist_ok=True)
        work_img = workdir / img_path.name
        shutil.copyfile(img_path, work_img)
        ret = kisis.segment(work_img, nl=nlines, overlap=0, pref__="$ISISROOT/IsisPreferences")
        os.remove(work_img)

        log.debug(f"{ret}")
        segment_metadata = pvl.loads(filter_progress(ret.stdout))
        
        # comes nested in a "results" group, trim it off
        segment_metadata = [s[1] for s in segment_metadata]
        
        glob_str = str(workdir / img_path.stem) + ".segment*"
        log.debug(f"globbing segments: glob({glob_str})")
        segments = sorted(glob(glob_str))

        log.debug(f"segments: {segments}")

        i = 0
        for s, meta in zip(segments, segment_metadata):
            i += 1
            meta["Path"] = s
            serial = kisis.getsn(s).stdout.split("\n")[0].strip()
            log.debug(f"sn: {serial}")
            meta["SN"] = serial 
            meta["Segment"] = i
            meta["Original"] = str(img_path)
        
        seg_dict_keys = [f"seg{n}" for n in range(1,len(segments)+1)]
        segment_dict = dict(zip(seg_dict_keys, segment_metadata))
        log.debug(f"Segment dict: {segment_dict}")
    except subprocess.CalledProcessError as err:
        print('Had an ISIS error:')
        print(' '.join(err.cmd))
        print(err.stdout)
        print(err.stderr)
        raise err
    return segment_dict


def generate_cnet(params, images):
    if isinstance(images["match"], list):
        images_match_n = images["match"][0]
    else:
        images_match_n = images["match"]
    match_segment_n = images_match_n["Segment"]
    from_segment_n = images["from"][0]["Segment"]

    print(images)
    workdir = Path(params["WORKDIR"])

    new_params = deepcopy(params)
    new_params.pop("NL")
    new_params.pop("MINAREA")
    new_params.pop("MINTHICKNESS") 
    new_params.pop("WORKDIR")


    # make sure none of these keys are still in the params
    new_params.pop("FROMLIST", None)
    new_params.pop("FROM", None)
    new_params["MATCH"] = images_match_n["Path"]

    og_onet = Path(params["ONET"])
    og_tolist = Path(params["TOLIST"])
    og_pointid = params["POINTID"]
    og_networkid = params["NETWORKID"]

    from_images = [image["Path"] for image in images["from"]]
    starting_lines = [image["StartingLine"] for image in images["from"]]
    log.debug(f"from images: {from_images}")
    
    match_stem = Path(new_params["MATCH"]).stem 

    fromlist_path = Path(workdir / f"from_images_segment{from_segment_n}.lis")
    from_stem = fromlist_path.stem

    if "debuglog" in new_params:
        og_log =  Path(new_params["DEBUGLOG"])
        new_log = og_log.parent / f"{og_log.stem}.{match_stem}_{from_stem}{og_log.suffix}" 
        new_params["DEBUGLOG"] = str(new_log)

    new_params["NETWORKID"] = og_networkid + f"_{match_segment_n}_{from_stem}"
    new_params["POINTID"] = og_pointid + f"_{match_segment_n}_{from_stem}"
    new_params["ONET"] = f"{og_onet.stem}_{match_segment_n}_{from_stem}.net"
    new_params["TOLIST"] = f"{og_tolist.stem}_{match_segment_n}_{from_stem}.lis"
    
    log.debug(new_params)

    overlapfromlist = workdir / f"{og_tolist.stem}_{match_segment_n}_{from_stem}_overlap_fromlist.lis"
    overlaptolist = workdir / f"{og_tolist.stem}_{match_segment_n}_{from_stem}.overlaps"
    
    # check for overlaps
    is_overlapping = []
    all_images = [*from_images, new_params["MATCH"]]
    print("ALL IMAGES: ", " ".join(all_images))
    kisis.fromlist.make(all_images, overlapfromlist)

    try:
        kisis.findimageoverlaps(fromlist=overlapfromlist, overlaplist=overlaptolist)
    except subprocess.CalledProcessError as err:
        print('Find Image Overlaps Had an ISIS error:')
        print(' '.join(err.cmd))
        print(err.stdout)
        print(err.stderr)
        return "No Overlaps From FindImageOverlaps"
        
    ret = kisis.overlapstats(fromlist=overlapfromlist, overlaplist=overlaptolist, pref__="$ISISROOT/IsisPreferences")
    stats = pvl.loads(filter_progress(ret.stdout))
    log.debug(f"overlap stats: {ret.stdout}")

    # # first, throw it out if there is no overlap whatsoever 
    # has_overlap = not any([len(k[1].get("NoOverlap", "")) == new_params["MATCH"] for k in stats])
    
    # # mask images
    # from_images = list(compress(from_images, is_overlapping))
    log.debug(f"From images overlapping Match: {from_images}")
    log.debug(f"Has overlaps list: {is_overlapping}")

    if from_images:
        log.debug(f"FROMLIST: {from_images}")

        if not fromlist_path.exists():
            log.debug(f"writing to: {fromlist_path}")
            kisis.fromlist.make(from_images, fromlist_path)
        else:
            log.debug(f"{fromlist_path} already exists")
        new_params["FROMLIST"] = str(fromlist_path)
        
        log.debug(f"Running FindFeatures with Params: {new_params}")

        try:
            ret = kisis.findfeatures(**new_params)
            log.debug(f"returned: {ret}")
        except subprocess.CalledProcessError as err:
            log.debug('Find Features Had an ISIS error:')
            log.debug(' '.join(err.cmd))
            log.debug(err.stdout)
            log.debug(err.stderr)

        if os.path.exists(new_params["ONET"]):
            segmented_net = cnet.from_isis(new_params["ONET"])

            # starting sample in inclusive, so subtract 1
            segmented_net.loc[segmented_net.serialnumber == images_match_n["SN"], "line"] += images_match_n["StartingLine"]-1 

            # offset the images 
            for k, image in enumerate(images["from"]):
                image_sn = image["SN"]

                # starting sample is inclusive, so subtract 1
                segmented_net.loc[segmented_net.serialnumber == image_sn, "line"] += starting_lines[k]-1
            cnet.to_isis(segmented_net, new_params["ONET"], targetname="moon")
            
            from_originals = [image["Original"] for image in images["from"]]
            return {"onet": new_params["ONET"], "original_images": from_originals}
        return "No Overlap"
    else: 
        return "No Overlap"

def merge(d1, d2, k): 
    """
    Merge two dictionary keys together such that values 
    always comes back as a list when keys overlap. 
    """
    # probably a cleaner way to do this

    v1 = d1.get(k, None)
    v2 = d2.get(k, None)

    if not v1:
        return v2 if isinstance(v2, list) else [v2]
    if not v2:
        return v1 if isinstance(v1, list) else [v1]
    if isinstance(v1, list) and not isinstance(v2, list):
        return v1+[v2]
    if not isinstance(v1, list) and isinstance(v2, list):
        return [v1]+v2
    if not isinstance(v1, list) and not isinstance(v2, list):
        return [v1]+[v2]
    if isinstance(v1, list) and isinstance(v2, list):
        return v1+v2


def findFeaturesSegment(ui, workdir):
    """
    findFeaturesSegment Calls FindFeatures on segmented images  

    findFeaturesSegment works by splitting the MATCH and FROM images into segments defined by NL with 0 
    pixel overlaps. The match image segments are then matched with every FROM/FROMLIST image semgnet that overlap 
    enough as defined by max/min overlap and area parameters. 

    Parameters
    ----------
    ui : astroset.UserInterface 
         UserInterface object containing user parameters
    images : dict
             dictionary of images to match. Format dictated by segment. 

    Returns
    -------
    dict
        dictionary containing output cnet and image list
    """
    if ui.GetBoolean("debug"):
        log.basicConfig(level=log.DEBUG)
    else: 
        log.basicConfig(level=log.INFO)    

    img_list = []
    if ui.WasEntered("From"):
        img_list = [ui.GetFileName("From")]
    elif ui.WasEntered("FromList"):
        img_list = read_cubelist(ui.GetFileName("FromList"))
    else:
        raise ValueError('**User Error** invalid command Line. Missing Parameter "from" or "fromlist"')    

    # Segment things 
    if ui.WasEntered("maxthreads"):
        nthreads = ui.GetInteger("maxthreads")
    else: 
        nthreads = int(multiprocessing.cpu_count())

    pool = ThreadPool(nthreads)
    starmap_args = []
    for image in img_list: 
        starmap_args.append([image, workdir, ui.GetInteger("NL")])
    output = pool.starmap_async(segment, starmap_args)
    pool.close()
    pool.join()
    output = output.get()
    match_segments = segment(os.path.abspath(ui.GetCubeName("match")), workdir, ui.GetInteger("NL"))
    
    if len(img_list) > 1:
        from_segments = reduce(lambda d1,d2: {k: merge(d1, d2, k) for k in set(d1)|set(d2)}, output)
    else:
        # img_list = 1
        from_segments = output[0]
        for seg, value in from_segments.items():
            og_value = value
            from_segments[seg] = [og_value]

    segment_paths = [s["Path"] for sublist in list(from_segments.values()) for s in sublist]
    segment_paths = segment_paths + [s["Path"] for s in list(match_segments.values())]
    
    # re-generate footprints
    pool = ThreadPool(nthreads)
    output = pool.map_async(footprintinit, segment_paths)
    pool.close()
    pool.join()
    output = output.get()
    log.debug(f"{output}")
    
    # Remove redundant overlapping pairs
    x = match_segments.values()
    y = from_segments.values()
    # x,y = (x,y) if len(x) > len(y) else (y,x)
    image_cartesian = list(itertools.product(x, y))
    image_sets = image_cartesian
    # for i in image_cartesian:
    #     if i[0][0]["Segment"] >= i[1]["Segment"]:
    #         image_sets.append(i) 

    # restructure things to be easier to manage
    job_dicts = []
    for im in image_sets:
        match = im[0]
        from_images = im[1]
        if not isinstance(from_images, list):
            # from_images must be list type
            from_images_list = []
            from_images_list.append(from_images)
            from_images = from_images_list
        job_dicts.append({
          "match" : match,
          "from" : from_images
        })

    # get params as a dictionary
    params = ui.GetParams()
    if is_workdir_temp:
        params["WORKDIR"] = workdir

    # findfeatures is already threaded, limit python threads by the maxthreads 
    # parameter, no maththreads_findfeatures * maxthreads_python <= maxthreads  
    pool = ThreadPool(int(nthreads/len(job_dicts)))
    starmap_args = list(zip([params]*len(job_dicts), job_dicts))
    output = pool.starmap_async(generate_cnet, starmap_args)
    pool.close()
    pool.join()
    output = output.get()
    log.debug(f"output: {output}")

    # merge the networks 
    onets = [o["onet"] for o in output if isinstance(o, dict)]
    log.debug(f"onets: {onets}")

    if len(onets) == 0:
        raise Exception("No Control Points Found!")

    onet_list = Path(ui.GetFileName("onet")).with_suffix(".segmented.lis")
    kisis.fromlist.make(onets, onet_list)
    
    # merge the filelists 
    tolists = [set(o["original_images"]) for o in output if isinstance(o, dict)] 

    final_images = set.union(*tolists)
    final_images.add(os.path.abspath(ui.GetCubeName("match")))

    log.debug(f"merged images: {final_images}")
    kisis.fromlist.make(final_images, Path(ui.GetFileName("tolist")))
    
    if len(onets) > 1: 
        try:
            kisis.cnetmerge(clist = onet_list, onet=ui.GetFileName("onet"), networkid=ui.GetAsString("networkid"), description=f"{ui.GetString('description')}")
        except subprocess.CalledProcessError as err:
            log.debug('Had an ISIS error:')
            log.debug(' '.join(err.cmd))
            log.debug(err.stdout)
            log.debug(err.stderr)
    elif len(onets) == 1: 
        # Dont merge 
        shutil.copy(onets[0], ui.GetFileName("onet"))

if __name__ == "__main__":
    ui = astroset.init_application(sys.argv)
    is_workdir_temp = not ui.WasEntered("WorkDir") 
    workdir = Path(tempfile.TemporaryDirectory().name) 
    if ui.WasEntered("Workdir"):
        workdir = Path(ui.GetFileName("Workdir"))
    
    try: 
        findFeaturesSegment(ui, workdir) 
    except Exception as e: 
        if is_workdir_temp:
            shutil.rmtree(workdir) 
        raise e 
    
    # log.info(f"COMPLETE, wrote: {ui.GetFileName("onet")}")
    if is_workdir_temp: 
        shutil.rmtree(workdir)
        log.info("Complete")
    else:
        log.info("Intermediate files written to %s", workdir)
