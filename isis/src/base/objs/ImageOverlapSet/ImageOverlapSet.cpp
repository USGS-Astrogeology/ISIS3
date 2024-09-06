/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "Cube.h"
#include "FileName.h"
#include "geos/operation/distance/DistanceOp.h"
#include "geos/util/IllegalArgumentException.h"
#include "geos/geom/Point.h"
//#include "geos/opOverlay.h"
#include "geos/operation/overlay/snap/SnapOverlayOp.h"

#include "IException.h"
#include "ImageOverlapSet.h"
#include "ImagePolygon.h"
#include "PolygonTools.h"
#include "Progress.h"
#include "SerialNumberList.h"

#include "QMessageBox"

using namespace std;

namespace Isis {

  /**
   * Create FindImageOverlaps object.
   *
   * Create an empty FindImageOverlaps object.
   *
   * @param continueOnError Whether or not this class throws exceptions in
   *                        addition to logging errors.
   * @see automaticRegistration.doc
   */
  ImageOverlapSet::ImageOverlapSet(bool continueOnError, bool useThread) {

    p_continueAfterError = continueOnError;
    p_writtenSoFar = 0;
    p_calculatedSoFar = -1;
    p_threadedCalculate = useThread;
    p_snlist = NULL;
  }


  /**
   * Delete this object.
   *
   * Delete the FindImageOverlaps object. The stored ImageOverlaps
   * will be deleted as well.
   */
  ImageOverlapSet::~ImageOverlapSet() {

    for (int i = 0; i < Size(); i++) {
      if (p_lonLatOverlaps[i]) delete p_lonLatOverlaps[i];
    }

    // This class should not retain ownership of p_snlist,
    //   so this member should not need destroyed.
  };


  /**
   * Create polygons of overlap from the images specified in the serial number
   * list. All polygons created by this class will be deleted when it is
   * destroyed, so callers should not delete the polygons returned by various
   * members.
   *
   * @param sns The serial number list to use when finding overlaps
   */
  void ImageOverlapSet::FindImageOverlaps(SerialNumberList &sns) {

    // Create an ImageOverlap for each image boundary
    for (int i = 0; i < sns.size(); i++) {
      // Open the cube
      Cube cube;
      try {
        cube.open(sns.fileName(i));
      }
      catch (IException &error) {
        std::string msg = "Unable to open cube for serial number [";
        msg += sns.serialNumber(i).toStdString() + "] filename [" + sns.fileName(i).toStdString() + "]";

        HandleError(error, &sns, msg);
      }

      // Read the bounding polygon
      ImagePolygon poly = cube.readFootprint();
      cube.close();
      // Create an ImageOverlap with the serial number and the bounding
      // polygon and save it
      geos::geom::MultiPolygon *tmp = PolygonTools::MakeMultiPolygon(poly.Polys());

      geos::geom::MultiPolygon *mp = NULL;

      // If footprint is invalid throw exception
      if(!tmp->isValid()) {
        delete tmp;
        tmp = NULL;
        std::string msg = "The image [" + sns.fileName(sns.serialNumber(i)).toStdString() +
                      "] has an invalid footprint";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }

      try {
        mp = PolygonTools::Despike(tmp);
      }
      catch (IException &e) {
        if (tmp->isValid()) {
          mp = tmp;
          tmp = NULL;
        }
        else {
          delete tmp;
          tmp = NULL;
          HandleError(e, &sns);
          continue;
        }
      }

      p_lonLatOverlapsMutex.lock();
      p_lonLatOverlaps.push_back(CreateNewOverlap(sns.serialNumber(i), mp));
      p_lonLatOverlapsMutex.unlock();

      if (mp) {
        delete mp;
        mp = NULL;
      }

      if (tmp) {
        delete tmp;
        tmp = NULL;
      }
    }

    // Despikes the polygons from the Serial Numbers prior to overlap
    //   determination
    DespikeLonLatOverlaps();

    if (p_threadedCalculate) {
      // Call FindAllOverlaps in other thread
      start();
    }
    else {
      // Determine the overlap between each boundary polygon
      FindAllOverlaps(&sns);
    }

  }


  /**
   * This method calculates image overlaps given a SerialNumberList and writes it
   * to the filename specified by outputFile. This method is internally optimized
   * and multi-threaded: the overlaps will NOT persist in memory after this method
   * is called. This object will be reset to its initial state when this is
   * called, and it is invalid to call this method if you have called other
   * methods first.
   *
   * This method is internally multi-threaded and more efficient than the others
   * for calculating overlaps.
   *
   * @param boundaries The files to find overlaps between
   * @param outputFile The output ImageOverlapSet file
   */
  void ImageOverlapSet::FindImageOverlaps(SerialNumberList &boundaries, QString outputFile) {

    // Do a common sense programmer check, this should be empty before we start
    if (!p_lonLatOverlaps.empty()) {
      string msg = "FindImageOverlaps(SerialNumberList&,QString) may not be called on " \
                   "an ImageOverlapSet which already contains overlaps.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    p_writtenSoFar = 0;
    p_calculatedSoFar = -1;

    p_snlist = &boundaries;

    FindImageOverlaps(boundaries);

    // While our exit condition is not true, call WriteImageOverlaps with the filename.
    // The WriteImageOverlaps call will block if it is waiting on calculations.
    while (p_calculatedSoFar != p_lonLatOverlaps.size()) {
      WriteImageOverlaps(outputFile);
    }

    // flush the output if we're still not done writing
    if (p_calculatedSoFar != p_writtenSoFar)
      WriteImageOverlaps(outputFile);

    // Wait for the calculation thread to actually exit,
    // this has more than likely already occurred.
    wait();

    // re-initialize object to original state
    p_lonLatOverlaps.clear();
    p_writtenSoFar = 0;
    p_calculatedSoFar = -1;
    p_threadedCalculate = false;
    p_snlist = NULL;
  }


  /** This is a strict pthread implementation of this class' multi-threading!
   *
  void ImageOverlapSet::FindImageOverlaps(SerialNumberList &boundaries, QString outputFile) {

    // Do a common sense programmer check, this should be empty before we start
    if (!p_lonLatOverlaps.empty()) {
      string msg = "FindImageOverlaps(SerialNumberList&,QString) may not be called on an ImageOverlapSet " \
                      "which already contains overlaps.";
      throw iException::Message(iException::Programmer, msg, _FILEINFO_);
    }

    p_writtenSoFar = 0;
    p_calculatedSoFar = -1;

    // This will enable using mutexes in the method calls where necessary.
    p_threadedCalculate = true;

    // We need to pass a this pointer to the thread AND the serial number list in order to have it
    //   calculate properly. Build the void* to pass in.
    void *data[] = {this, &boundaries};

    // This is the thread that will be calculating the overlaps. Our current thread will be the one writing
    //   to the output file so synchronization at exit is not an issue.
    pthread_t calculateThread;

    // Enter the initialization phase: don't try to write until the other thread says initialization is done by
    //   unlocking this mutex.
    pthread_mutex_lock(&initDataMutex);

    // Create the other thread - it will initialize variables (the ImageOverlap list), unlock the initDataMutex,
    //   and proceed to calculate. After each polygon is calculated the calculating mutex will also be
    //   unlocked in order to allow I/O if possible. When done calculatedSoFar == p_lonLatOverlaps.size() and the calculating mutex
    //   is unlocked.
    pthread_create(&calculateThread, NULL, FindImageOverlapsThreadStart, &data);

    // this will let us pass when initialization is done
    pthread_mutex_lock(&initDataMutex);

    // Final unlock of the initialization mutex - we locked it to get into this code
    pthread_mutex_unlock(&initDataMutex);

    // While our exit condition is not true, call WriteImageOverlaps with the filename. The
    //   WriteImageOverlaps call will block if it is waiting on calculations.
    while(p_calculatedSoFar != (int)p_lonLatOverlaps.size()) {
      WriteImageOverlaps(outputFile);
    }

    // Wait for the calculation thread to actually exit,
    //   this has more than likely already occurred.
    void *result;
    pthread_join(calculateThread, &result);

    // re-initialize object to original state
    p_lonLatOverlaps.clear();
    p_writtenSoFar = 0;
    p_calculatedSoFar = -1;
    p_threadedCalculate = false;
  }*/


  /**
   * This is the method that is called when a thread is spawned by
   * FindImageOverlaps(...). It simply calls FindImageOverlaps with a
   * SerialNumberList and exits.
   *
   * @param data An array of the form {ImageOverlapSet* instance, SerialNumberList *snlist)
   *
   * @return void* Returns null
   *
  void *ImageOverlapSet::FindImageOverlapsThreadStart(void *data) {
    ImageOverlapSet *instance = (ImageOverlapSet *) ((void**)data)[0];
    SerialNumberList *snlist  = (SerialNumberList *)((void**)data)[1];
    instance->FindImageOverlaps( *snlist );
    pthread_exit(NULL);
  }*/


  /**
   * Create polygons of overlap from the polygons specified. The serial numbers
   * and the polygons are assumed to be parallel arrays. The original polygons
   * passed as arguments are copied, so the ownership of the originals remains
   * with the caller.
   *
   * @param sns The serial number list to use when finding overlaps
   * @param polygons The polygons which are to be used when finding overlaps
   *
   * @see automaticRegistration.doc
   */
  void ImageOverlapSet::FindImageOverlaps(std::vector<QString> sns,
                                          std::vector<geos::geom::MultiPolygon *> polygons) {

    if (sns.size() != polygons.size()) {
      string message = "Invalid argument sizes. Sizes must match.";
      throw IException(IException::Programmer, message, _FILEINFO_);
    }

    // Create one ImageOverlap for each image sn
    for (unsigned int i = 0; i < sns.size(); ++i) {
      p_lonLatOverlapsMutex.lock();
      p_lonLatOverlaps.push_back(CreateNewOverlap(sns[i], polygons[i]));
      p_lonLatOverlapsMutex.unlock();
    }

    // Despikes the polygons from the Serial Numbers prior to overlap determination
    DespikeLonLatOverlaps();

    // Determine the overlap between each boundary polygon
    FindAllOverlaps();
  }


  /**
   * Create polygons of overlap from the file specified.
   *
   * @param filename The file to read the image overlaps from
   */
  void ImageOverlapSet::ReadImageOverlaps(const QString &filename) {

    QString file = QString::fromStdString(FileName(filename.toStdString()).expanded());

    try {
      // Let's get an istream pointed at our file
      std::ifstream inStream;
      QByteArray fileArray = file.toLatin1();
      inStream.open(fileArray.constData(), fstream::in | fstream::binary);

      while (!inStream.eof()) {
        p_lonLatOverlapsMutex.lock();
        p_lonLatOverlaps.push_back(new ImageOverlap(inStream));
        p_lonLatOverlapsMutex.unlock();
      }

      inStream.close();
    }
    catch (IException &e) {
      p_lonLatOverlapsMutex.unlock();
      std::string msg = "The overlap file [" + filename.toStdString() + "] does not contain a "
                    "valid list of image overlaps";
      throw IException(e, IException::Unknown, msg, _FILEINFO_);
    }
    catch (...) {
      p_lonLatOverlapsMutex.unlock();
      std::string msg = "The overlap file [" + filename.toStdString() + "] does not contain a "
                    "valid list of image overlaps";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }
  }


  /**
   * This method inserts or overwrites a polygon in the overlap list based on
   * parameters. "poly" is inserted at or after position if insert == true. "poly"
   * is set at position if insert == false. Serial numbers from sncopy will be
   * added to the new/existing ImageOverlap. This method WILL DELETE poly.
   *
   * This method will attempt to despike poly. This method will return true if the
   * operation was valid - if inserting and the polygon ended up being
   * empty, this will still return true.
   *
   * @param poly The geos polygon to insert/set
   * @param position The position to insert/set
   * @param sncopy Serial numbers to copy to the ImageOverlap
   * @param insert True if inserting new overlap
   *
   * @return bool True if operation was valid
   */
  bool ImageOverlapSet::SetPolygon(geos::geom::Geometry *poly,
                                   int position,
                                   ImageOverlap *sncopy,
                                   bool insert) {

    bool success = false;
    geos::geom::MultiPolygon *multiPolygon = PolygonTools::MakeMultiPolygon(poly);
    delete poly;
    poly = NULL;

    if (!multiPolygon->isValid() ||
        (multiPolygon->getArea() < 1.0e-10 && !multiPolygon->isEmpty())) {
      delete multiPolygon;
      multiPolygon = Isis::globalFactory->createMultiPolygon().release();
    }

    if (position > p_lonLatOverlaps.size()) {
      position = p_lonLatOverlaps.size();
    }

    try {
      if (!multiPolygon->isEmpty()) {
        geos::geom::MultiPolygon *despiked = PolygonTools::Despike(multiPolygon);
        delete multiPolygon;
        multiPolygon = despiked;
      }
    }
    catch (IException &) {
    }

    if (multiPolygon->isValid() &&
        (multiPolygon->isEmpty() || multiPolygon->getArea() > 1.0e-14)) {
      if (!insert) {
        p_lonLatOverlaps.at(position)->SetPolygon(multiPolygon);

        if (sncopy) {
          AddSerialNumbers(p_lonLatOverlaps.at(position), sncopy);
        }
      }
      else if (!multiPolygon->isEmpty()) {
        ImageOverlap *imageOverlap = new ImageOverlap();
        imageOverlap->SetPolygon(multiPolygon);
        delete multiPolygon;
        multiPolygon = NULL;

        if (sncopy) {
          AddSerialNumbers(imageOverlap, sncopy);
        }

        // Insert could cause a reallocation of the overlap list, so lock it with
        // the writing code so that we don't conflict
        p_lonLatOverlapsMutex.lock();
        p_lonLatOverlaps.insert(p_lonLatOverlaps.begin() + position, imageOverlap);
        p_lonLatOverlapsMutex.unlock();
      }

      success = true;
    }

    return success;
  }


  /**
   * Write polygons of overlap to the file specified.
   *
   * @param filename The file to write the image overlaps to
   */
  void ImageOverlapSet::WriteImageOverlaps(const QString &filename) {

    QString file = QString::fromStdString(FileName(filename.toStdString()).expanded());
    bool failed = false;
    bool noOverlaps = false;

    if (p_threadedCalculate) {
      p_calculatePolygonMutex.lock();
    }

    // Let's get an ostream pointed at our file
    std::ofstream outStream;

    try {
      QByteArray fileArray = file.toLatin1();
      if (p_writtenSoFar == 0) {
        outStream.open(fileArray.constData(), fstream::out | fstream::trunc | fstream::binary);
      }
      else {
        outStream.open(fileArray.constData(), fstream::out | fstream::app | fstream::binary);
      }

      failed |= outStream.fail();

      for (int overlap = p_writtenSoFar; !failed && overlap <= p_calculatedSoFar; overlap++) {
        // Let's not try anything during a possible reallocate
        p_lonLatOverlapsMutex.lock();

        if (p_lonLatOverlaps.size() == 0) {
          noOverlaps = true;
        }
        else {
          if (overlap < p_lonLatOverlaps.size() && p_lonLatOverlaps[overlap]) {

            if (!p_lonLatOverlaps[overlap]->Polygon()->isEmpty()) {

              if (p_writtenSoFar) {
                outStream << std::endl;
              }

              p_lonLatOverlaps[overlap]->Write(outStream);
            }

            delete p_lonLatOverlaps[overlap];
            p_lonLatOverlaps[overlap] = NULL;
            p_writtenSoFar ++;
          }
        }
        p_lonLatOverlapsMutex.unlock();
      }

      failed |= outStream.fail();
      outStream.close();

      failed |= outStream.fail();
    }
    catch (...) {
      failed = true;
    }
    outStream.close();

    /**
     * Don't wait for an unlock from FindImageOverlaps(...) if we're done
     * calculating.
     */
    if (p_calculatedSoFar == p_lonLatOverlaps.size()) {
      if (p_threadedCalculate && !noOverlaps) {
        p_calculatePolygonMutex.tryLock();
        p_calculatePolygonMutex.unlock();
      }
    }

    if (failed) {
      p_calculatePolygonMutex.tryLock();
      p_calculatePolygonMutex.unlock();
      std::string msg = "Unable to write the image overlap list to [" + filename.toStdString() + "]";
      throw IException(IException::Io, msg, _FILEINFO_);
    }
    else if (noOverlaps) {
      p_calculatePolygonMutex.tryLock();
      p_calculatePolygonMutex.unlock();
      std::string msg = "No overlaps were found.";
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }


  /**
   * Find the overlaps between all the existing ImageOverlap Objects
   *
   * @param snlist The serialnumber list relating to the overlaps described by the
   *               current known ImageOverlap objects or NULL
   */
  void ImageOverlapSet::FindAllOverlaps(SerialNumberList *snlist) {

    bool foundOverlap = false;
    if (p_lonLatOverlaps.size() <= 1) return;

    Progress p;
    p.SetText("Calculating Image Overlaps");
    p.SetMaximumSteps(p_lonLatOverlaps.size() - 1);
    p.CheckStatus();

    geos::geom::MultiPolygon *emptyPolygon = Isis::globalFactory->createMultiPolygon().release();

    // Compare each polygon with all of the others
    for (int outside = 0; outside < p_lonLatOverlaps.size() - 1; ++outside) {
      p_calculatedSoFar = outside - 1;

      // unblock the writing process after every 10 polygons if we need to write
      if (p_calculatedSoFar % 10 == 0 && (!snlist || (p_lonLatOverlaps.size() > snlist->size()))) {
        if (p_threadedCalculate) {
          p_calculatePolygonMutex.tryLock();
          p_calculatePolygonMutex.unlock();
        }
      }

      // Intersect the current polygon (from the outside loop) with all others
      // below it
      for (int inside = outside + 1; inside < p_lonLatOverlaps.size(); ++inside) {
        try {
          if (p_lonLatOverlaps.at(outside)->HasAnySameSerialNumber(*p_lonLatOverlaps.at(inside)))
            continue;

          // We know these are valid because they were filtered early on
          const geos::geom::MultiPolygon *poly1 = p_lonLatOverlaps.at(outside)->Polygon();
          const geos::geom::MultiPolygon *poly2 = p_lonLatOverlaps.at(inside)->Polygon();

          // Check to see if the two poygons are equivalent.
          // If they are, then we can get rid of one of them
          if (PolygonTools::Equal(poly1, poly2)) {
            p_lonLatOverlapsMutex.lock();
            AddSerialNumbers(p_lonLatOverlaps[outside], p_lonLatOverlaps[inside]);
            p_lonLatOverlaps.erase(p_lonLatOverlaps.begin() + inside);
            p_lonLatOverlapsMutex.unlock();
            inside --;
            continue;
          }

          // We can get empty polygons in our list sometimes; try to avoid extra processing
          if (poly2->isEmpty() || poly2->getArea() < 1.0e-14) {
            p_lonLatOverlapsMutex.lock();
            p_lonLatOverlaps.erase(p_lonLatOverlaps.begin() + inside);
            p_lonLatOverlapsMutex.unlock();
            inside --;
            continue;
          }

          geos::geom::Geometry *intersected = NULL;
          try {
            intersected = PolygonTools::Intersect(poly1, poly2);
          }
          catch (IException &e) {
            intersected = NULL;
            std::string error = "Intersection of overlaps failed.";

            // We never want to double seed, so we must delete one or both
            //   of these polygons because they more than likely have an intersection
            //   that we simply can't calculate.
            double outsideArea = poly1->getArea();
            double insideArea = poly2->getArea();
            double areaRatio = std::min(outsideArea, insideArea) /
                               std::max(outsideArea, insideArea);

            // If one of the polygons is < 1% the area of the other,
            //   then only throw out the small one to
            //   try to minimize the impact of this failure.
            if (areaRatio < 0.1) {
              if (poly1->getArea() > poly2->getArea()) {
                error += " The first polygon will be removed.";
                HandleError(e, snlist, error, inside, outside);
                p_lonLatOverlapsMutex.lock();
                p_lonLatOverlaps.erase(p_lonLatOverlaps.begin() + inside);
                p_lonLatOverlapsMutex.unlock();
                inside --;
              }
              else {
                error += " The second polygon will be removed.";
                HandleError(e, snlist, error, inside, outside);
                p_lonLatOverlapsMutex.lock();
                p_lonLatOverlaps.erase(p_lonLatOverlaps.begin() + outside);
                p_lonLatOverlapsMutex.unlock();
                inside = outside;
              }
            }
            else {
              error += " Both polygons will be removed to prevent the "
                       "possibility of double counted areas.";
              HandleError(e, snlist, error, inside, outside);
              p_lonLatOverlapsMutex.lock();
              p_lonLatOverlaps.erase(p_lonLatOverlaps.begin() + inside);
              p_lonLatOverlaps.erase(p_lonLatOverlaps.begin() + outside);
              p_lonLatOverlapsMutex.unlock();
              inside = outside;
            }

            continue;
          }

          if (intersected->isEmpty() || intersected->getArea() < 1.0e-14) {
            delete intersected;
            intersected = NULL;
            continue;
          }

          // We are only interested in overlaps that result in polygon(s)
          // and not any that are lines or points, so create a new multipolygon
          // with only the polygons of overlap
          geos::geom::MultiPolygon *overlap = NULL;
          try {
            overlap = PolygonTools::Despike(intersected);

            delete intersected;
            intersected = NULL;
          }
          catch (IException &e) {
            if (!intersected->isValid()) {
              delete intersected;
              intersected = NULL;

              HandleError(e, snlist, "", inside, outside);
              continue;
            }
            else {
              overlap = PolygonTools::MakeMultiPolygon(intersected);

              delete intersected;
              intersected = NULL;
            }
          }
          catch (geos::util::GEOSException *exc) {
            delete intersected;
            intersected = NULL;
            HandleError(exc, snlist, "", inside, outside);
            continue;
          }

          if (!overlap->isValid()) {
            delete overlap;
            overlap = NULL;
            HandleError(snlist, "Intersection produced invalid overlap area", inside, outside);
            continue;
          }

          // is there really overlap?
          if (overlap->isEmpty() || overlap->getArea() < 1.0e-14) {
            delete overlap;
            overlap = NULL;
            continue;
          }

          // poly1 is completely inside poly2
          if (PolygonTools::Equal(poly1, overlap)) {
            geos::geom::Geometry *tmpGeom = NULL;
            try {
              tmpGeom = PolygonTools::Difference(poly2, poly1);
            }
            catch (IException &e) {
              HandleError(e, snlist, "Differencing overlap polygons failed."
                                     "The first polygon will be removed.", inside, outside);

              // Delete outside polygon directly and reset outside loop
              //   - current outside is thrown out!
              p_lonLatOverlapsMutex.lock();
              p_lonLatOverlaps.erase(p_lonLatOverlaps.begin() + outside);
              p_lonLatOverlapsMutex.unlock();
              inside = outside;
              continue;
            }
            if (SetPolygon(tmpGeom, inside) &&
                SetPolygon(overlap, outside, p_lonLatOverlaps[inside]))
              foundOverlap = true;
          }
          // poly2 is completely inside poly1
          else if (PolygonTools::Equal(poly2, overlap)) {
            geos::geom::Geometry *tmpGeom = NULL;
            try {
              tmpGeom = PolygonTools::Difference(poly1, poly2);
            }
            catch (IException &e) {
              HandleError(e, snlist, "Differencing overlap polygons failed."
                                     "The second polygon will be removed.", inside, outside);

              // Delete inside polygon directly and process next inside
              p_lonLatOverlapsMutex.lock();
              p_lonLatOverlaps.erase(p_lonLatOverlaps.begin() + inside);
              p_lonLatOverlapsMutex.unlock();
              inside --;
              continue;
            }
            if (SetPolygon(tmpGeom, outside) &&
                SetPolygon(overlap, inside, p_lonLatOverlaps[outside]))
              foundOverlap = true;
          }
          // There is partial overlap
          else {
            // Subtract overlap from poly1 and set poly1 to the result
            geos::geom::Geometry *tmpGeom = NULL;
            try {
              tmpGeom = PolygonTools::Difference(poly1, overlap);
            }
            catch (IException &e) {
              tmpGeom = NULL;
            }

            // If we failed to subtract overlap, try to subtract poly2 from poly1
            // and set poly1 to the result
            try {
              if (tmpGeom == NULL) {
                tmpGeom = PolygonTools::Difference(poly1, poly2);
              }
            }
            catch (IException &e) {
              tmpGeom = NULL;
              HandleError(e, snlist, "Differencing overlap polygons failed", inside, outside);
              continue;
            }

            if (!SetPolygon(tmpGeom, outside)) {
              if (SetPolygon(Isis::globalFactory->createMultiPolygon().release(), outside))
                foundOverlap = true;
            }

            int oldSize = p_lonLatOverlaps.size();
            if (SetPolygon(overlap, inside + 1, p_lonLatOverlaps[outside], true)) {
              int newSize = p_lonLatOverlaps.size();
              int newSteps = newSize - oldSize;
              p.AddSteps(newSteps);
              foundOverlap = true;
              if (newSize != oldSize) inside++;
            }
          } // End of partial overlap else
        }
        // Collections are illegal as intersection argument
        catch (IException &e) {
          HandleError(e, snlist, "Unable to find overlap.", inside, outside);
        }
        // Collections are illegal as intersection argument
        catch (geos::util::IllegalArgumentException *ill) {
          HandleError(NULL, snlist, "Unable to find overlap", inside, outside);
        }
        catch (geos::util::GEOSException *exc) {
          HandleError(exc, snlist, "Unable to find overlap", inside, outside);
        }
        catch (...) {
          HandleError(snlist, "Unknown Error: Unable to find overlap", inside, outside);
        }
      }

      p.CheckStatus();
    }

    p_calculatedSoFar = p_lonLatOverlaps.size();
    delete emptyPolygon;

    // Do not write empty overlap files
    if (foundOverlap == false) {
      p_lonLatOverlapsMutex.lock();
      p_lonLatOverlaps.clear();
      p_lonLatOverlapsMutex.unlock();
    }

    // unblock the writing process
    // Check first if the the thread is still locked
    // to avoid undefined behavior
    p_calculatePolygonMutex.tryLock();
    p_calculatePolygonMutex.unlock();
  }


  /**
   * Add the serial numbers from the second overlap to the first
   *
   * Note: Need to check for existence of a SN before adding it
   *
   * @param to    The object to receive the new serial numbers
   * @param from  The object to copy the serial numbers from
   */
  void ImageOverlapSet::AddSerialNumbers(ImageOverlap *to, ImageOverlap *from) {

    for (int i = 0; i < from->Size(); i++) {
      QString s = (*from)[i];
      to->Add(s);
    }
  }


  /**
   * Create an overlap item to hold the overlap poly and its SN
   *
   * @param serialNumber The serial number
   * @param latLonPolygon The object to copy the serial numbers from
   */
  ImageOverlap *ImageOverlapSet::CreateNewOverlap(QString serialNumber,
      geos::geom::MultiPolygon *latLonPolygon) {

    return new ImageOverlap(serialNumber, *latLonPolygon);
  }


  /**
   * Return the overlaps that have a specific serial number.
   *
   * Search the existing ImageOverlap objects for all that have the
   * serial numbers associated with them. Note: This could be costly when many
   * overlaps exist.
   *
   * @param serialNumber The serial number to be search for in all existing ImageOverlaps
   *
   * @return List of related ImageOverlap objects, ownership is retained by ImageOverlapSet*
   */
  std::vector<ImageOverlap *> ImageOverlapSet::operator[](QString serialNumber) {

    vector<ImageOverlap *> matches;

    // Look at all the ImageOverlaps we have and return the ones that
    // have this sn
    for (int ov = 0; ov < p_lonLatOverlaps.size(); ++ov) {
      for (int sn = 0; sn < p_lonLatOverlaps[ov]->Size(); ++sn) {
        if ((*p_lonLatOverlaps[ov])[sn] == serialNumber) {
          matches.push_back(p_lonLatOverlaps[ov]);
        }
      }
    }

    return matches;
  }


  /**
   * If a problem occurred when searching for image overlaps,
   * this method will handle it.
   *
   * @param e Isis Exception representing the problem
   * @param snlist Serial number list to get file information from
   * @param msg Error message
   * @param overlap1 First problematic overlap
   * @param overlap2 Second problematic overlap
   */
  void ImageOverlapSet::HandleError(IException &e,
                                    SerialNumberList *snlist,
                                    std::string msg,
                                    int overlap1, int overlap2) {

    PvlGroup err("ImageOverlapError");

    if (overlap1 >= 0 && overlap1 < p_lonLatOverlaps.size()) {
      PvlKeyword serialNumbers("PolySerialNumbers");
      PvlKeyword filename("FileNames");
      PvlKeyword polygon("Polygon");

      for (int i = 0; i < p_lonLatOverlaps.at(overlap1)->Size(); i++) {
        serialNumbers += (*p_lonLatOverlaps.at(overlap1))[i].toStdString();

        if (snlist != NULL) {
          filename += snlist->fileName((*p_lonLatOverlaps.at(overlap1))[i]).toStdString();
        }
      }
      polygon += p_lonLatOverlaps.at(overlap1)->Polygon()->toString().c_str();

      err += serialNumbers;

      if (filename.size() != 0) {
        err += filename;
      }

      err += polygon;
    }

    if (overlap2 >= 0 && overlap1 < p_lonLatOverlaps.size() &&
        overlap2 < p_lonLatOverlaps.size()) {
      PvlKeyword serialNumbers("PolySerialNumbers");
      PvlKeyword filename("FileNames");
      PvlKeyword polygon("Polygon");

      for (int i = 0; i < p_lonLatOverlaps.at(overlap2)->Size(); i++) {
        serialNumbers += (*p_lonLatOverlaps.at(overlap2))[i].toStdString();

        if (snlist != NULL) {
          filename += snlist->fileName((*p_lonLatOverlaps.at(overlap2))[i]).toStdString();
        }
      }
      polygon += p_lonLatOverlaps.at(overlap2)->Polygon()->toString().c_str();

      err += serialNumbers;

      if (filename.size() != 0) {
        err += filename;
      }

      err += polygon;
    }

    err += PvlKeyword("Error", e.what());

    if (!msg.empty()) {
      err += PvlKeyword("Description", msg);
    }

    p_errorLog.push_back(err);

    if (!p_continueAfterError) throw;
  }


  /**
   * If a problem occurred when searching for image overlaps,
   * this method will handle it.
   *
   * @param exc GEOS Exception representing the problem
   * @param snlist Serial number list to get file information from
   * @param msg Error message
   * @param overlap1 First problematic overlap
   * @param overlap2 Second problematic overlap
   */
  void ImageOverlapSet::HandleError(geos::util::GEOSException *exc,
                                    SerialNumberList *snlist,
                                    std::string msg,
                                    int overlap1, int overlap2) {

    PvlGroup err("ImageOverlapError");

    if (overlap1 >= 0 && overlap1 < p_lonLatOverlaps.size()) {
      PvlKeyword serialNumbers("PolySerialNumbers");
      PvlKeyword filename("FileNames");

      for (int i = 0; i < p_lonLatOverlaps.at(overlap1)->Size(); i++) {
        serialNumbers += (*p_lonLatOverlaps.at(overlap1))[i].toStdString();

        if (snlist != NULL) {
          filename += snlist->fileName((*p_lonLatOverlaps.at(overlap1))[i]).toStdString();
        }
      }

      err += serialNumbers;

      if (filename.size() != 0) {
        err += filename;
      }
    }

    if (overlap2 >= 0 && overlap1 < p_lonLatOverlaps.size() &&
        overlap2 < p_lonLatOverlaps.size()) {
      PvlKeyword serialNumbers("PolySerialNumbers");
      PvlKeyword filename("FileNames");

      for (int i = 0; i < p_lonLatOverlaps.at(overlap2)->Size(); i++) {
        serialNumbers += (*p_lonLatOverlaps.at(overlap2))[i].toStdString();

        if (snlist != NULL) {
          filename += snlist->fileName((*p_lonLatOverlaps.at(overlap2))[i]).toStdString();
        }
      }

      err += serialNumbers;

      if (filename.size() != 0) {
        err += filename;
      }
    }

    err += PvlKeyword("Error", exc->what());

    if (!msg.empty()) {
      err += PvlKeyword("Description", msg);
    }

    p_errorLog.push_back(err);

    delete exc;

    if (!p_continueAfterError) {
      throw IException(IException::Programmer, err["Description"][0], _FILEINFO_);
    }
  }


  /**
   * If a problem occurred when searching for image overlaps,
   * this method will handle it.
   *
   * @param snlist Serial number list to get file information from
   * @param msg Error message
   * @param overlap1 First problematic overlap
   * @param overlap2 Second problematic overlap
   */
  void ImageOverlapSet::HandleError(SerialNumberList *snlist,
                                    std::string msg,
                                    int overlap1, int overlap2) {

    PvlGroup err("ImageOverlapError");

    if (overlap1 >= 0 && overlap1 < p_lonLatOverlaps.size()) {
      PvlKeyword serialNumbers("PolySerialNumbers");
      PvlKeyword filename("FileNames");

      for (int i = 0; i < p_lonLatOverlaps.at(overlap1)->Size(); i++) {
        serialNumbers += (*p_lonLatOverlaps.at(overlap1))[i].toStdString();

        if (snlist != NULL) {
          filename += snlist->fileName((*p_lonLatOverlaps.at(overlap1))[i]).toStdString();
        }
      }

      err += serialNumbers;

      if (filename.size() != 0) {
        err += filename;
      }
    }

    if (overlap2 >= 0 && overlap1 < p_lonLatOverlaps.size() &&
        overlap2 < p_lonLatOverlaps.size()) {
      PvlKeyword serialNumbers("PolySerialNumbers");
      PvlKeyword filename("FileNames");

      for (int i = 0; i < p_lonLatOverlaps.at(overlap2)->Size(); i++) {
        serialNumbers += (*p_lonLatOverlaps.at(overlap2))[i].toStdString();

        if (snlist != NULL) {
          filename += snlist->fileName((*p_lonLatOverlaps.at(overlap2))[i]).toStdString();
        }
      }

      err += serialNumbers;

      if (filename.size() != 0) {
        err += filename;
      }
    }

    err += PvlKeyword("Description", msg);

    p_errorLog.push_back(err);

    if (!p_continueAfterError) {
      throw IException(IException::Programmer, err["Description"][0], _FILEINFO_);
    }
  }


  /**
   * Despikes all of the overlaps in p_lonLatOverlaps.
   * Currently (2009-03-19), this fixes spiked multipolygons generated by
   * footprintinit, prior to calculating overlaps.
   */
  void ImageOverlapSet::DespikeLonLatOverlaps() {

    for (int i = 0; i < Size(); i++) {
      try {
        p_lonLatOverlaps[i]->SetPolygon(PolygonTools::Despike(p_lonLatOverlaps[i]->Polygon()));
      }
      catch (IException &e) {
      }
    }
  }
}
