/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "cnetcombinept.h"

#include <QFile>
#include <QHash>
#include <QtGlobal>
#include <QTextStream>
#include <QScopedPointer>
#include <QSet>
#include <QStringList>

// boost library
#include <boost/foreach.hpp>
#include <boost/assert.hpp>

#include "nanoflann.hpp"

#include "Application.h"
#include "ControlNet.h"
#include "MeasurePoint.h"
#include "ControlPointMerger.h"
#include "FileList.h"
#include "FileName.h"
#include "IString.h"
#include "PointCloud.h"
#include "PointCloudTree.h"
#include "ProcessByLine.h"
#include "Progress.h"
#include "SerialNumber.h"
#include "SpecialPixel.h"
#include "SurfacePoint.h"
#include "IException.h"

using namespace std;
namespace Isis{
  /** Check point for validity */
  inline bool isValid(const ControlPoint *point ) {
    if ( !point ) { return ( false ); }
    return ( !(point->IsIgnored() || point->IsInvalid() || point->IsRejected()) );
  }


  /** Check point for merging worthiness */
  inline bool isWorthy(const ControlPoint *point ) {
    if ( !point ) { return ( false ); }
    return ( isValid(point) && !point->IsEditLocked() );
  }

  /** Check measure for validity */
  inline bool isValid(const ControlMeasure *m)  {
     return ( !( m->IsIgnored() || m->IsRejected() ) );
  }

  /**
   * Recursive function that merges the logs when points are merged together.
   * This ensures that previous merges are retained when a point is merged
   * multiple times. This function searches the merge log for any previous merges
   * into the control points that are now being merged into another point. The old
   * merges are then removed from the log and appended to the new merge.
   *
   * @param(in/out) mergeLog The merge log to check/update
   * @param newMerges        The set of new point IDs to append
   */
  QSet<QString> combineMerges(QHash<QString, QSet<QString>> &mergeLog, QSet<QString> newMerges) {
    QSet<QString> combinedMerges(newMerges);
    BOOST_FOREACH ( QString pointId, newMerges ) {
      // Recursively append everything previously merged into this point
      if (mergeLog.contains(pointId)) {
        combinedMerges.unite(combineMerges(mergeLog, mergeLog.take(pointId)));
      }
    }
    return combinedMerges;
  }

  // Control network manager
  typedef QList<QSharedPointer<ControlNet> >    ControlNetList;

  // ControlNet point cloud types.
  typedef MeasurePoint                       PointType;
  typedef PointCloud<PointType>              CNetPointCloud;
  typedef PointCloudTree<PointType>          CNetPointCloudTree;
  typedef QSharedPointer<CNetPointCloudTree> CubeMeasureTree;

  void cnetcombinept(UserInterface &ui, Pvl *log) {

    // We will be processing by line
    ProcessByLine pbl;

    QStringList cnetfiles;
    int nBase(0);
    if ( ui.WasEntered("CNETBASE") ) {
      cnetfiles.append(ui.GetAsString("CNETBASE"));
      nBase++;
    }

    int nFrom(0);
    if ( ui.WasEntered("CNETFROM") ) {
      cnetfiles.append(ui.GetAsString("CNETFROM"));
      nFrom++;
    }

    int nList(0);
    if ( ui.WasEntered("CNETLIST") ) {
      FileList list_o_nets(ui.GetFileName("CNETLIST"));
      if (list_o_nets.size() < 1) {
        QString mess = "The file provided for CNETLIST, [";
        mess += ui.GetAsString("CNETLIST");
        mess += "] is empty.";
        throw IException(IException::User, mess, _FILEINFO_);
      }

      BOOST_FOREACH ( FileName cfile, list_o_nets ) {
        cnetfiles.append( cfile.original() );
        nList++;
      }
    }

    // Check for any files at all
    int totalNetFiles = nBase + nFrom + nList;
    if ( totalNetFiles <= 0 ) {
      QString mess = "No input networks files provided!";
      throw IException(IException::User, mess, _FILEINFO_);
    }

    //---------------------------------------------------------------------------
    //  Load all the input control networks
    //---------------------------------------------------------------------------
    Progress progress;
    progress.SetText("Loading");
    progress.SetMaximumSteps(cnetfiles.size() );
    progress.CheckStatus();

    // Collect some stuff from input nets for the output net
    QString netid;
    QString target;
    QString description;

    BigInt allPoints(0);
    BigInt validPoints(0);
    QHash<QString, QList<ControlMeasure *> > cube_measures;
    QList<ControlPoint *> all_points;

    BOOST_FOREACH ( QString cfile, cnetfiles ) {
  #if defined(DEBUG)
      std::cout << "\nLoading " << cfile << "...\n";
      Progress cnet_progress;
      QScopedPointer<ControlNet> cnet( new ControlNet(cfile, &cnet_progress) );
  #else
      QScopedPointer<ControlNet> cnet( new ControlNet(cfile) );
  #endif
      if ( netid.isEmpty() ) {
        netid = cnet->GetNetworkId();
      }
      if ( target.isEmpty() ) {
        target = cnet->GetTarget();
      }
      if ( description.isEmpty() ) {
        description = cnet->Description();
      }

      // Now get list of all cube serials and add all to list
      QList<QString> serials = cnet->GetCubeSerials();
      BOOST_FOREACH ( QString sn, serials) {
        QList<ControlMeasure *> measures = cnet->GetMeasuresInCube(sn);
        BOOST_ASSERT( measures.size() > 0 );

        // Eliminate ignored measures (and its assocaited point)
        QList<ControlMeasure *> goods;
        BOOST_FOREACH (ControlMeasure *m, measures ) {
          if ( isValid(m) ) {
            ControlPoint *point = m->Parent();
            if ( isWorthy(point) ) {
              goods.append(m);
            }
          }
        }

        // Now insert valid measures associated with serial (cube) if we have any
        if ( goods.size() > 0 ) {
          cube_measures[sn].append(goods);
          allPoints += goods.size();  // Measures count
        }
      }

      // Take ownership of all points and let the cnet file close
      validPoints += cnet->GetNumValidPoints();
      all_points.append( cnet->take() );
      progress.CheckStatus();
    }

    // Report status of network
    int cube_measures_size = cube_measures.size();
    std::cout << "\nTotal Points:   " << all_points.size() << "\n";
    std::cout << "Valid Points:   " << validPoints << "\n";
    std::cout << "Total Measures: " << allPoints << "\n";
    std::cout << "Total Cubes:    " << cube_measures_size << "\n\n";

    // Now write out the list of SNs if requested
    if ( ui.WasEntered("TOSN") ) {

      FileName filename( ui.GetFileName("TOSN") );
      QFile logfile(filename.expanded());
      if ( !logfile.open(QIODevice::WriteOnly | QIODevice::Truncate |
                         QIODevice::Text | QIODevice::Unbuffered) ) {
        QString mess = "Unable to open/create serial number file " + filename.name();
        throw IException(IException::User, mess, _FILEINFO_);
      }

      QTextStream lout(&logfile);
      QHashIterator<QString, QList<ControlMeasure *> > sns(cube_measures);
      while ( sns.hasNext()  ) {
        sns.next();
        lout << sns.key() << "\n";
      }

      logfile.close();
    }


    //---------------------------------------------------------------------------
    // Construct the kd-trees that assocate all the measures with points for
    // each cube.
    //---------------------------------------------------------------------------
    progress.SetText("making trees");
    progress.SetMaximumSteps( cube_measures.size() );
    progress.CheckStatus();

    // Create the kd-tree lookup for each measure in each cube
    QHash<QString, CubeMeasureTree> measure_clouds;
    QHashIterator<QString, QList<ControlMeasure *> > sn_m(cube_measures);
    int kd_nodes = ui.GetInteger("KDNODES");
    while ( sn_m.hasNext()  ) {
      sn_m.next();

      // Generate a kd-tree for all measures in each cube for distance comparisons
      QScopedPointer<CNetPointCloud> cloud(new CNetPointCloud(sn_m.value(), sn_m.key()));
      CubeMeasureTree cloud_t(new CNetPointCloudTree(cloud.take(), kd_nodes ) );
      measure_clouds.insert(sn_m.key(), cloud_t);

      progress.CheckStatus();
    }

    // Retain for future reference. We intend to release the trees when we are
    // done with them as we may need the memory.
    // int measure_clouds_size = measure_clouds.size();

    //---------------------------------------------------------------------------
    //  Now perform the merge. Iterate through all points evaluating each
    //  measure  to see if same measure exists in any other point within
    //  the IMAGETOL limit.
    //---------------------------------------------------------------------------
    progress.SetText("merging");
    progress.SetMaximumSteps( all_points.size() );
    progress.CheckStatus();

    //  Measure distance tolerance
    double image_tolerance  = ui.GetDouble("IMAGETOL");
    double search_radius_sq = image_tolerance * image_tolerance;

    // Optional logging
    // Note: These is can store a significant number of strings, but all of them
    //       already exist elsewhere so it's very lightweight as long as we don't
    //       modify any of them.
    QHash<QString, QSet<QString>> mergeLog;
    QHash<QString, int> startingPointSizes;
    QHash<QString, int> endingPointSizes;
    bool logMerges = ui.WasEntered("LOGFILE");

    //  Run through all valid points. Note they may be invalided as processing
    //  is done through mergers, so validity must be checked at each point.
    BigInt nfound(0);
    BigInt nMerged(0);
    BOOST_FOREACH ( ControlPoint *point, all_points ) {
      BigInt localFound(0);
      // Don't consider ignored or edit locked points
      if ( isWorthy( point ) ) {

        // Get all valid measures only in the point
        QList<ControlMeasure *> v_measures = point->getMeasures( true );

        // Log starting point size
        if (logMerges) {
          startingPointSizes.insert(point->GetId(), point->GetNumMeasures());
        }

        int p_merged(0);
        BOOST_FOREACH ( ControlMeasure *v_m, v_measures ) {
          PointType m_p(v_m);     // This associates the measure to its point
          if ( m_p.isValid() ) {  // Valid point? If not, its likely merged already
            CubeMeasureTree m_cloud = measure_clouds[v_m->GetCubeSerialNumber()];
            BOOST_ASSERT( m_cloud != 0 );
            QList<PointType> m_points = m_cloud->radius_query(m_p, search_radius_sq);
            ControlPointMerger merger(image_tolerance);
            p_merged += merger.apply(point, m_points);
            localFound = merger.size();
            nfound   += localFound;

            // Log any points that were merged
            if (logMerges && localFound > 0) {
              QHash<QString, QSet<QString>>::iterator logIt = mergeLog.find(point->GetId());
              // point hasn't had any points merged into it yet
              if (logIt == mergeLog.end()) {
                mergeLog.insert( point->GetId(), combineMerges(mergeLog, merger.mergedPoints()) );
              }
              // point has already had points merged into it
              else {
                logIt.value().unite( combineMerges(mergeLog, merger.mergedPoints()) );
              }
            }
          }
        }
        nMerged += p_merged;
      }
      progress.CheckStatus();
    }

    // All done with the heavy lifting, so free resources as memory may be
    // needed later.
    cube_measures.clear();
    measure_clouds.clear();

    //---------------------------------------------------------------------------
    //  Screen the control points for reduction of content in the output network
    //  file but don't create it in this loop - its very expensive.
    //---------------------------------------------------------------------------
    progress.SetText("screening/cleaning/building network");
    progress.SetMaximumSteps( all_points.size() );
    progress.CheckStatus();

    // User options
    bool cleannet = ui.GetBoolean("CLEANNET");
    bool cleanmeasures = ui.GetBoolean("CLEANMEASURES");
    int minmeasures = ui.GetInteger("MINMEASURES");

    // Set up control net here so we can complete all processing in this step
    QScopedPointer<ControlNet> cnet;
    if ( ui.WasEntered("ONET") ) {
      // std::cout << "\nWriting network...\n";
      // Set up the output control network
      cnet.reset( new ControlNet() );
      if ( ui.WasEntered("NETWORKID") ) {
        netid = ui.GetString("NETWORKID");
      }

      cnet->SetNetworkId(netid);
      cnet->SetUserName(Application::UserName());

      if ( ui.WasEntered("DESCRIPTION") ) {
        description = ui.GetString("DESCRIPTION");
      }

      cnet->SetDescription(description);
      cnet->SetCreatedDate(Application::DateTime());
      cnet->SetTarget(target);
  #if defined(HAS_WRITE_ONLY_OPTION)
      cnet->setWriteOnly();
  #endif
    }

    // Check to see if we want to reset the apriori surface to the best
    // available measure in the point
    bool setaprioribest = ui.GetBoolean("SETAPRIORIBEST");

    BigInt oPoints(0);
    BigInt nRemoved(0);
    BigInt nMinMeasures(0);
    BigInt vPoints(0);
    QHash<QString, int> pointIds; // To protect against redundant point ids
    for ( int i = 0 ; i < all_points.size() ; i++) {

      ControlPoint *m_p = all_points[i];

      // Check for redunant point id here
      QString pid = m_p->GetId();
      if (  pointIds.contains( pid ) ) {
        int pcount = pointIds.value(pid);
        QString id = pid +  "_" + QString::number(pcount);
        m_p->SetId(id);
        pointIds[pid] = ++pcount;
      }
      else {
        pointIds.insert(pid, 1);
      }

      if ( isValid(m_p) ) {
        vPoints++;

        // Processes measures if requested
        if ( true == cleanmeasures ) {

          QList<ControlMeasure *> measures = m_p->getMeasures( false );
          BOOST_FOREACH ( ControlMeasure *m, measures) {
            if ( !isValid(m) ) {
              m_p->Delete(m);
              nRemoved++;
            }
          }
        }

        //  Check for valid measure constraints
        if ( (m_p->GetNumValidMeasures() < minmeasures) && (!m_p->IsEditLocked()) ) {
          m_p->SetIgnored( true );
          nMinMeasures++;
        }
      }

      // Log final point size
      if (logMerges) {
        endingPointSizes.insert(pid, m_p->GetNumMeasures());
      }

      // Save invalid points?  We are not going to create the network if only if
      // requested by the user with the good points as its a very expensive
      // operation.
      if ( true == cleannet ) {
        if ( isValid(m_p) ) {  // Handle valid points
          if ( !cnet.isNull() ) {
            if ( (true == setaprioribest) && !m_p->IsEditLocked() ) {
              m_p->SetAprioriSurfacePoint(m_p->GetBestSurfacePoint());
            }
            cnet->AddPoint(m_p);
            oPoints++;
          }
          else {  // If not creating control network, ensure points are deleted
            delete (m_p);
          }
        }
        else {  // Not a valid point, delete it
          delete (m_p);
        }
      }
      else {  // Handle points when not cleaning
        if ( !cnet.isNull() ) {
          if ( (true == setaprioribest) && !m_p->IsEditLocked() ) {
            m_p->SetAprioriSurfacePoint(m_p->GetBestSurfacePoint());
          }

          cnet->AddPoint(m_p);
          oPoints++;
        }
        else { // If not creating control network, ensure points are deleted
          delete (m_p);
        }
      }

      progress.CheckStatus();
    }

    //---------------------------------------------------------------------------
    // Write the resulting control network to the specfied ONET file. We will now
    //  create the network formally. If not requested, don't forget to free all
    //  remaining points (done by ControlNet otherwise).
    //---------------------------------------------------------------------------
    if ( ui.WasEntered("ONET") ) {
      // Make it so!
      cnet->Write( ui.GetAsString("ONET") );
    }

    // Write out the merge log
    if (logMerges) {
      FileName mergeLogFileName( ui.GetFileName("LOGFILE") );
      QFile mergeLogfile(mergeLogFileName.expanded());
      if ( !mergeLogfile.open(QIODevice::WriteOnly | QIODevice::Truncate |
                         QIODevice::Text | QIODevice::Unbuffered) ) {
        QString mess = "Unable to open/create merge log file " + mergeLogFileName.name();
        throw IException(IException::User, mess, _FILEINFO_);
      }

      QTextStream mergeLogStream(&mergeLogfile);
      mergeLogStream << "pointID,startNumMeasures,endNumMeasures,mergedIDs" << "\n";
      for (auto mergeLogIt = mergeLog.constBegin(); mergeLogIt != mergeLog.constEnd(); mergeLogIt++) {
        mergeLogStream << mergeLogIt.key() << ","
                       << startingPointSizes[mergeLogIt.key()] << ","
                       << endingPointSizes[mergeLogIt.key()] << ","
                       << QStringList(mergeLogIt.value().values()).join(" ")
                       << "\n";
      }

      mergeLogfile.close();
    }

    // Write out a report
    int pMerged = validPoints - vPoints;
    PvlGroup summary("Summary");
    summary += PvlKeyword("TotalCubes",        toString(cube_measures_size));
    summary += PvlKeyword("TotalInputPoints",  toString(all_points.size()));
    summary += PvlKeyword("TotalOutputPoints", toString(oPoints));
    summary += PvlKeyword("PointsMerged",      toString(pMerged));
    summary += PvlKeyword("PointsEvaluated",   toString(nfound));
    summary += PvlKeyword("TotalMeasures",     toString(allPoints));
    summary += PvlKeyword("MeasuresMerged",    toString(nMerged));
    summary += PvlKeyword("MeasuresDeleted",   toString(nRemoved));
    summary += PvlKeyword("MinimumMeasures",   toString(nMinMeasures));
    Application::Log(summary);

    pbl.EndProcess();
  }
}
