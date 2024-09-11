#include "FeatureNomenclature.h"

#include <QDebug>
#include <QDomElement>
#include <QGridLayout>
#include <QLabel>
#include <QList>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QPair>
#include <QUrl>
#include <QUrlQuery>

#include "Distance.h"
#include "IException.h"
#include "IString.h"
#include "iTime.h"
#include "Latitude.h"
#include "Longitude.h"

namespace Isis {

  /**
   * Instantiate a feature nomenclature. This prepares to make network requests.
   */
  FeatureNomenclature::FeatureNomenclature() {
    m_networkMgr = NULL;
    m_request = NULL;
    m_features = NULL;

    m_networkMgr = new QNetworkAccessManager;
    connect(m_networkMgr, SIGNAL(finished(QNetworkReply *)),
            this, SLOT(requestFinished(QNetworkReply *)));

    m_request = new QNetworkRequest;
    m_request->setUrl(
        QUrl("https://planetarynames.wr.usgs.gov/SearchResults"));
    m_request->setRawHeader("User-Agent",
                            "Mozilla/5.0 (X11; Linux i686; rv:6.0) "
                            "Gecko/20100101 Firefox/6.0");
    m_request->setHeader(QNetworkRequest::ContentTypeHeader,
                         "application/x-www-form-urlencoded");
    m_lastQuery = true;
  }


  /**
   * Copy a feature nomenclature. Data being queried currently will not be
   *   available in the copy, but ready results will be available.
   *
   * @param other The feature nomenclature to copy
   */
  FeatureNomenclature::FeatureNomenclature(const FeatureNomenclature &other) {
    m_networkMgr = NULL;
    m_request = NULL;
    m_features = NULL;

    m_networkMgr = new QNetworkAccessManager;

    connect(m_networkMgr, SIGNAL(finished(QNetworkReply *)),
            this, SLOT(requestFinished(QNetworkReply *)));

    m_request = new QNetworkRequest;
    m_lastQuery = other.m_lastQuery;

    if (other.m_features)
      m_features = new QList<Feature>(*other.m_features);
  }


  /**
   * Frees allocated memory.
   */
  FeatureNomenclature::~FeatureNomenclature() {
    delete m_networkMgr;
    m_networkMgr = NULL;

    delete m_request;
    m_request = NULL;

    delete m_features;
    m_features = NULL;
  }


   /**
   * Makes sure the longitudinal ranges are correct. If the range intersects with the 0 line the
   *   range is split into two ranges, the minimum to 360 and 0 to the maximum. Then it runs a
   *   query on each range.
   *
   * @param target The target for which features might be present
   * @param startLat The minimum latitude of the ground range to search
   * @param startLon The minimum longitude of the ground range to search
   * @param endLat The maximum latitude of the ground range to search
   * @param endLon The maximum longitude of the ground range to search
   */
  void FeatureNomenclature::queryFeatures(QString target,
                                          Latitude startLat, Longitude startLon,
                                          Latitude endLat, Longitude endLon) {

    QList< QPair<Longitude, Longitude> > range = Longitude::to360Range(startLon, endLon);

    if (!range.isEmpty()) {
      startLon = range[0].first;
      endLon = range[0].second;
    }

    if (range.size() > 1) {

      Longitude startLon2 = range[1].first;
      Longitude endLon2 = range[1].second;

      runQuery(target, startLat, startLon, endLat, endLon);
      runQuery(target, startLat, startLon2, endLat, endLon2);

      m_lastQuery = false;
    }
    else {
      runQuery(target, startLat, startLon, endLat, endLon);

      m_lastQuery = true;
    }
  }


  /**
   * This gives you the features found in all of the queries so far. If all of
   *   the queries are finished, then this list will contain all of the results.
   *   The list will be empty if no queries have succeeded so far.
   *
   * @return a cumulative list of all of the features found so far.
   */
  QList<FeatureNomenclature::Feature> FeatureNomenclature::features() const {
    QList<Feature> featureList;

    if (m_features)
      featureList = *m_features;

    return featureList;
  }


  /**
   * Test if any understandable results have been received from the nomenclature
   *   database.
   *
   * @return True if any queries have succeeded.
   */
  bool FeatureNomenclature::hasResult() const {
    return (m_features != NULL);
  }


  /**
   * Swap the instances *this and other. This cannot throw an exception.
   *
   * @param other The FeatureNomenclature to trade data with.
   */
  void FeatureNomenclature::swap(FeatureNomenclature &other) {
    std::swap(m_networkMgr, other.m_networkMgr);
    std::swap(m_request, other.m_request);
    std::swap(m_features, other.m_features);
  }


  /**
   * This takes on the data from rhs. Active queries will not be copied. This is
   *   exception safe.
   *
   * @param rhs The FeatureNomenclature on the right hand side of the '='
   * @return *this
   */
  FeatureNomenclature &FeatureNomenclature::operator=(
      const FeatureNomenclature &rhs) {
    FeatureNomenclature tmp(rhs);
    swap(tmp);
    return *this;
  }


  /**
   * Compare the diameter of two features. This is very useful for sorting with
   *   std::sort(). If lhs > rhs, then this returns true. This considers valid data
   *   > invalid data.
   *
   * @param lhs The left hand side of the '>' operator
   * @param rhs The right hand side of the '>' operator
   * @return lhs > rhs
   */
  bool FeatureNomenclature::featureDiameterGreaterThan(
      const FeatureNomenclature::Feature &lhs,
      const FeatureNomenclature::Feature &rhs) {
    Distance lhsDiameter = lhs.diameter();
    Distance rhsDiameter = rhs.diameter();

    bool greaterThan = false;
    if (lhsDiameter.isValid() && rhsDiameter.isValid()) {
      greaterThan = (lhsDiameter > rhsDiameter);
    }
    else {
      greaterThan = lhsDiameter.isValid();
    }

    return greaterThan;
  }


  /**
   * Construct a feature with no data.
   */
  FeatureNomenclature::Feature::Feature() {
    m_xmlRepresenation = NULL;
    m_approvalStatus = NoStatus;
  }


  /**
   * Construct a feature with the data encapsulated inside of the XML.
   */
  FeatureNomenclature::Feature::Feature(QDomElement searchResultFeature, IAUStatus status) {
    m_xmlRepresenation = NULL;
    m_xmlRepresenation = new QDomElement(searchResultFeature);
    m_approvalStatus = status;
  }


  /**
   * Copy a feature. This is a shallow copy.
   *
   * @param other The feature to copy
   */
  FeatureNomenclature::Feature::Feature(const Feature &other) {
    m_xmlRepresenation = NULL;
    // QDomElement does a shallow copy.
    m_approvalStatus = other.m_approvalStatus;
    m_xmlRepresenation = new QDomElement(*other.m_xmlRepresenation);
  }


  /**
   * Clean up allocated memory by this feature.
   */
  FeatureNomenclature::Feature::~Feature() {
    delete m_xmlRepresenation;
    m_xmlRepresenation = NULL;
  }


  /**
   * This converts the data in this feature to a widget. All of the information
   *   returned by the server is encapsulated in this widget. There are no
   *   controls, this is a display only.
   *
   * @return A widget that displays all of the feature information
   */
  QWidget *FeatureNomenclature::Feature::toWidget() const {
    QWidget *widget = new QWidget;

    QGridLayout *layout = new QGridLayout;
    widget->setLayout(layout);

    int row = 0;

    QLabel *titleLabel = new QLabel("<h2>Feature Details</h2>");
    layout->addWidget(titleLabel, row, 0, 1, 2);
    row++;

    QList< QPair<QString, QString (FeatureNomenclature::Feature::*)() const> >
        displayValues;

    QPair<QString, QString (FeatureNomenclature::Feature::*)() const>
        displayValue;

    displayValue.first = "Feature Name:";
    displayValue.second = &FeatureNomenclature::Feature::displayName;
    displayValues.append(displayValue);

//     displayValue.first = "Feature Name (clean):";
//     displayValue.second = &FeatureNomenclature::Feature::cleanName;
//     displayValues.append(displayValue);

    displayValue.first = "Feature ID:";
    displayValue.second = &FeatureNomenclature::Feature::id;
    displayValues.append(displayValue);

    displayValue.first = "Target:";
    displayValue.second = &FeatureNomenclature::Feature::target;
    displayValues.append(displayValue);

    displayValue.first = "System:";
    displayValue.second = &FeatureNomenclature::Feature::system;
    displayValues.append(displayValue);

    displayValue.first = "Control Network:";
    displayValue.second = &FeatureNomenclature::Feature::controlNet;
    displayValues.append(displayValue);

    displayValue.first = "Diameter:";
    displayValue.second = &FeatureNomenclature::Feature::diameterString;
    displayValues.append(displayValue);

    displayValue.first = "Originating Continent:";
    displayValue.second =
        &FeatureNomenclature::Feature::originatingContinent;
    displayValues.append(displayValue);

    displayValue.first = "Originating Ethnicity:";
    displayValue.second =
        &FeatureNomenclature::Feature::originatingEthnicity;
    displayValues.append(displayValue);

    displayValue.first = "Feature Type:";
    displayValue.second =
        &FeatureNomenclature::Feature::featureType;
    displayValues.append(displayValue);

    displayValue.first = "Center Latitude:";
    displayValue.second =
        &FeatureNomenclature::Feature::centerLatitudeString;
    displayValues.append(displayValue);

    displayValue.first = "Center Longitude:";
    displayValue.second =
        &FeatureNomenclature::Feature::centerLongitudeString;
    displayValues.append(displayValue);

    displayValue.first = "Northern Latitude:";
    displayValue.second =
        &FeatureNomenclature::Feature::northernLatitudeString;
    displayValues.append(displayValue);

    displayValue.first = "Southern Latitude:";
    displayValue.second =
        &FeatureNomenclature::Feature::southernLatitudeString;
    displayValues.append(displayValue);

    displayValue.first = "Eastern Longitude:";
    displayValue.second =
        &FeatureNomenclature::Feature::easternLongitudeString;
    displayValues.append(displayValue);

    displayValue.first = "Western Longitude:";
    displayValue.second =
        &FeatureNomenclature::Feature::westernLongitudeString;
    displayValues.append(displayValue);

    displayValue.first = "Approval Date:";
    displayValue.second =
        &FeatureNomenclature::Feature::approvalDate;
    displayValues.append(displayValue);

    displayValue.first = "Approval Status:";
    displayValue.second =
        &FeatureNomenclature::Feature::approvalStatus;
    displayValues.append(displayValue);

    displayValue.first = "Last Updated:";
    displayValue.second =
        &FeatureNomenclature::Feature::lastUpdated;
    displayValues.append(displayValue);

    displayValue.first = "Reference:";
    displayValue.second =
        &FeatureNomenclature::Feature::referenceString;
    displayValues.append(displayValue);

    displayValue.first = "Origin:";
    displayValue.second =
        &FeatureNomenclature::Feature::origin;
    displayValues.append(displayValue);

    displayValue.first = "URL:";
    displayValue.second =
        &FeatureNomenclature::Feature::referenceUrlString;
    displayValues.append(displayValue);

    for (int i = 0; i < displayValues.count(); i++) {
      QLabel *titleLabel = new QLabel(displayValues[i].first);
      QLabel *valueLabel = new QLabel( (this->*(displayValues[i].second))() );
      valueLabel->setOpenExternalLinks(true);
      valueLabel->setWordWrap(true);

      if (valueLabel->text() != "") {
        layout->addWidget(titleLabel, row, 0);
        layout->addWidget(valueLabel, row, 1);
      }
      else {
        delete titleLabel;
        delete valueLabel;
      }

      row++;
    }

    return widget;
  }


  /**
   * @return The feature ID (typically a number)
   */
  QString FeatureNomenclature::Feature::id() const {
    return getTagText("id");
  }


  /**
   * @return The feature name (not always compatible with QString)
   */
  QString FeatureNomenclature::Feature::name() const {
    return getTagText("name");
  }


  /**
   * @return The 'clean' feature name (non-ascii characters cleaned up)
   */
  QString FeatureNomenclature::Feature::cleanName() const {
    return getTagText("cleanName");
  }


  /**
   * @return The source control network for the feature position information.
   *           This is implied by what's currently available for targets, since
   *           the database does not return this information.
   */
  QString FeatureNomenclature::Feature::controlNet() const {
    QString targetStr = target();
    QString cnet = "";

    if (targetStr.toUpper() == "MOON")
      cnet = "LOLA";
    else if (targetStr.toUpper() == "MARS")
      cnet = "MDIM 2.1";
    else if (targetStr.toUpper() == "MERCURY")
      cnet = "Preliminary MESSENGER";

    return cnet;
  }


  /**
   * @return The recommended feature name to display to the users
   */
  QString FeatureNomenclature::Feature::displayName() const {
    QString nameString = name();
    QString cleanNameString = cleanName();

    QString displayNameString = nameString;

    if (nameString != cleanNameString)
      displayNameString = nameString + " (" + cleanNameString + ")";

    return displayNameString;
  }


  /**
   * @return The target name
   */
  QString FeatureNomenclature::Feature::target() const {
    return getTagText("target");
  }


  /**
   * @return The target's system (i.e. Target: Moon, System: Earth)
   */
  QString FeatureNomenclature::Feature::system() const {
    return getTagText("system");
  }


  /**
   * @return The feature's diameter
   */
  Distance FeatureNomenclature::Feature::diameter() const {
    Distance result;

    try {
      result = Distance(getTagText("diameter").toDouble(),
                        Distance::Kilometers);
    }
    catch (IException &) {
    }

    return result;
  }


  /**
   * @return The feature's diameter as a human readable string.
   */
  QString FeatureNomenclature::Feature::diameterString() const {
    return diameter().toString();
  }


  /**
   * @return The feature's center latitude.
   */
  Latitude FeatureNomenclature::Feature::centerLatitude() const {
    Latitude result;

    try {
      result = Latitude(getTagText("centerlatitude").toDouble(), Angle::Degrees);
    }
    catch (IException &) {
    }

    return result;
  }


  /**
   * @return The feature's center latitude as a human readable string.
   */
  QString FeatureNomenclature::Feature::centerLatitudeString() const {
    return centerLatitude().toString();
  }


  /**
   * @return The feature's center longitude.
   */
  Longitude FeatureNomenclature::Feature::centerLongitude() const {
    Longitude result;

    try {
      result = Longitude(getTagText("centerlongitude").toDouble(),
                         Angle::Degrees);
    }
    catch (IException &) {
    }

    return result;
  }


  /**
   * @return The feature's center longitude as a human readable string.
   */
  QString FeatureNomenclature::Feature::centerLongitudeString() const {
    return centerLongitude().toString();
  }


  /**
   * @return The feature's northernmost (max) latitude.
   */
  Latitude FeatureNomenclature::Feature::northernLatitude() const {
    Latitude result;

    try {
      result = Latitude(getTagText("northernLatitude").toDouble(), Angle::Degrees);
    }
    catch (IException &) {
    }

    return result;
  }


  /**
   * @return The feature's northernmost (max) latitude as a human readable
   *         string.
   */
  QString FeatureNomenclature::Feature::northernLatitudeString() const {
    return northernLatitude().toString();
  }


  /**
   * @return The feature's southernmost (min) latitude.
   */
  Latitude FeatureNomenclature::Feature::southernLatitude() const {
    Latitude result;

    try {
      result = Latitude(getTagText("southernLatitude").toDouble(), Angle::Degrees);
    }
    catch (IException &) {
    }

    return result;
  }


  /**
   * @return The feature's southernmost (min) latitude as a human readable
   *         string.
   */
  QString FeatureNomenclature::Feature::southernLatitudeString() const {
    return southernLatitude().toString();
  }


  /**
   * @return The feature's easternmost (max) longitude.
   */
  Longitude FeatureNomenclature::Feature::easternLongitude() const {
    Longitude result;

    try {
      result = Longitude(getTagText("easternLongitude").toDouble(),
                         Angle::Degrees);
    }
    catch (IException &) {
    }

    return result;
  }


  /**
   * @return The feature's easternmost (max) longitude as a human readable
   *         string.
   */
  QString FeatureNomenclature::Feature::easternLongitudeString() const {
    return easternLongitude().toString();
  }


  /**
   * @return The feature's westernmost (min) longitude.
   */
  Longitude FeatureNomenclature::Feature::westernLongitude() const {
    Longitude result;

    try {
      result = Longitude(getTagText("westernLongitude").toDouble(),
                         Angle::Degrees);
    }
    catch (IException &) {
    }

    return result;
  }


  /**
   * @return The feature's westernmost (min) longitude as a human readable
   *         string.
   */
  QString FeatureNomenclature::Feature::westernLongitudeString() const {
    return westernLongitude().toString();
  }


  /**
   * @return The continent from which the person who named the feature
   *         originated.
   */
  QString FeatureNomenclature::Feature::originatingContinent() const {
    return getTagText("continent");
  }


  /**
   * @return The ethnicity of the person who named the feature.
   */
  QString FeatureNomenclature::Feature::originatingEthnicity() const {
    return getTagText("ethnicity");
  }


  /**
   * @return The IAU approval status of the feature.
   */
  QString FeatureNomenclature::Feature::approvalStatus() const {
    return getTagText("approvalstatus");
  }


  /**
   * @return The approval date of the feature.
   */
  QString FeatureNomenclature::Feature::approvalDate() const {
    return getTagText("approvaldate");
  }


  /**
   * @return The type of feature (for example, crater)
   */
  QString FeatureNomenclature::Feature::featureType() const {
    return getTagText("featuretype");
  }


  /**
   * @return The feature's reference (bibliography) information
   */
  QString FeatureNomenclature::Feature::referenceString() const {
    return getTagText("reference");
  }


  /**
   * @return The feature's origin
   */
  QString FeatureNomenclature::Feature::origin() const {
    return getTagText("origin");
  }


  /**
   * @return The feature's last updated time as a string.
   */
  QString FeatureNomenclature::Feature::lastUpdated() const {
    return getTagText("lastUpdated");
  }


  /**
   * @return The feature's online URL.
   */
  QUrl FeatureNomenclature::Feature::referenceUrl() const {
    return QUrl("http://planetarynames.wr.usgs.gov/Feature/" + id());
  }


  /**
   * @return The feature's online URL as a HTML string (it is hyperlinked).
   */
  QString FeatureNomenclature::Feature::referenceUrlString() const {
    return "<a href='" + referenceUrl().toString() + "'>" +
             referenceUrl().toString() +
           "</a>";
  }


  /**
   * @return The feature's enumerated approval status.
   */
  FeatureNomenclature::IAUStatus FeatureNomenclature::Feature::status() const {
    return m_approvalStatus;
  }


  /**
   * Swap the member data of this feature with another feature.
   *
   * @param other The feature to swap with.
   */
  void FeatureNomenclature::Feature::swap(Feature &other) {
    std::swap(m_xmlRepresenation, other.m_xmlRepresenation);
    std::swap(m_approvalStatus, other.m_approvalStatus);

  }


  /**
   * Assign the values of this feature from the values of rhs.
   *
   * @param rhs The feature on the right hand side of the '=' operator
   * @return *this
   */
  FeatureNomenclature::Feature &FeatureNomenclature::Feature::operator=(
      const Feature &rhs) {
    Feature copy(rhs);
    swap(copy);
    return *this;
  }


  /**
   * Get the string value of an element of the XML. Returns an empty string if
   *   anything goes wrong.
   *
   * @param tagName the XML tag name, from the nomenclature server, of what you
   *        want
   * @return The data in the tag
   */
  QString FeatureNomenclature::Feature::getTagText(QString tagName) const {
    QString text;

    if (m_xmlRepresenation) {
      QDomNodeList nodes =
          m_xmlRepresenation->elementsByTagName(tagName);

      if (nodes.count())
        text = nodes.at(0).toElement().text().trimmed();
    }

    return text;
  }


  /**
   * This is called when a query is done. This adds the features to our feature
   *   list and emits featuresIdentified().
   *
   * @param reply The server's response to our request, containing features in
   *     XML form.
   */
  void FeatureNomenclature::requestFinished(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {

      QString errorMsg;
      int errorLine;
      int errorCol;

      QDomDocument xmlResultDocument;
      if (xmlResultDocument.setContent(reply->readAll(),
                                       &errorMsg, &errorLine, &errorCol)) {
        for (QDomNode node = xmlResultDocument.firstChild();
             !node.isNull();
             node = node.nextSibling()) {
          QDomElement element = node.toElement();
          if (element.tagName() == "searchresults") {
            readSearchResults(element);
          }
        }
      }
      else if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 301) {
        IString msg = "The URL has been permanently moved to " +
                      reply->attribute(QNetworkRequest::RedirectionTargetAttribute)
                      .toUrl().toString().toStdString();
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }
      else {
      QMessageBox::warning(NULL, "Failed to read nomenclature database result",
                           "An error occurred when parsing the data sent back "
                           "from the nomenclature database. "
                           "The XML result was invalid. The parse is [" +
                           errorMsg + "] on line [" +
                           QString::number(errorLine) +"], column [" +
                           QString::number(errorCol) + "]");
      }
    }
    else {
      QMessageBox::warning(NULL, "Failed to query nomenclature database",
                           "An error occurred when querying the nomenclature "
                           "database for features that intersect the queried "
                           "ground range. Please make sure you have an active "
                           "internet connection. The error returned was [" +
                           reply->errorString() + "]");
    }

    reply->deleteLater();

    if (m_lastQuery) {
      emit featuresIdentified(this);
    }
    m_lastQuery = true;
  }


  /**
   * This is a helper method for requestFinished. This takes the search results
   *   and creates a Feature for each equivalent XML feature element.
   *
   * @param xmlSearchResults The searchresults XML from the nomenclature server.
   */
  void FeatureNomenclature::readSearchResults(QDomElement xmlSearchResults) {

    if (!m_features)
      m_features = new QList<Feature>;

    for (QDomNode node = xmlSearchResults.firstChild();
         !node.isNull();
         node = node.nextSibling()) {
      QDomElement element = node.toElement();
      QString approvalID = element.childNodes().item(15).toElement().attribute("id");

      if (element.tagName() == "feature") {

        if(approvalID == "5") {
           m_statusApproval = Approved;
        }
        else if(approvalID == "6") {
          m_statusApproval = Dropped;
        }
        else if(approvalID == "7") {
          m_statusApproval = Unapproved;
        }
        else {
          m_statusApproval = NoStatus;
        }

        m_features->append(Feature(element, m_statusApproval));
      }
    }
  }


   /**
   * Query the nomenclature database for features inside the given range on the
   *   target. When the last query for the cube is done, featuresIdentified()
   *   will be emitted.
   *
   * You can call query as many times as you'd like, but the results will be
   *   cumulative. Errors are reported in the form of a message box.
   *
   * @param target The target for which features might be present
   * @param startLat The minimum latitude of the ground range to search
   * @param startLon The minimum longitude of the ground range to search
   * @param endLat The maximum latitude of the ground range to search
   * @param endLon The maximum longitude of the ground range to search
   */
  void FeatureNomenclature::runQuery(QString target,
                                     Latitude startLat, Longitude startLon,
                                     Latitude endLat, Longitude endLon) {

    QUrlQuery formQuery;

    // List of XML fields we want from the server
    formQuery.addQueryItem("additionalInfoColumn", "true");
    formQuery.addQueryItem("approvalDateColumn", "true");
    formQuery.addQueryItem("approvalStatusColumn", "true");
    formQuery.addQueryItem("centerLatLonColumn", "true");
    formQuery.addQueryItem("cleanFeatureNameColumn", "true");
    formQuery.addQueryItem("contEthColumn", "true");
    formQuery.addQueryItem("coordSystemColumn", "true");
    formQuery.addQueryItem("diameterColumn", "true");
    formQuery.addQueryItem("featureIDColumn", "true");
    formQuery.addQueryItem("featureNameColumn", "true");
    formQuery.addQueryItem("featureTypeCodeColumn", "true");
    formQuery.addQueryItem("featureTypeColumn", "true");
    formQuery.addQueryItem("lastUpdatedColumn", "true");
    formQuery.addQueryItem("latLonColumn", "true");
    formQuery.addQueryItem("originColumn", "true");
    formQuery.addQueryItem("quadColumn", "true");
    formQuery.addQueryItem("referenceColumn", "true");
    formQuery.addQueryItem("targetColumn", "true");

    // Data units
    formQuery.addQueryItem("is_0_360", "true");
    formQuery.addQueryItem("is_planetographic", "false");
    formQuery.addQueryItem("is_positive_east", "true");

    // Format parameters
    formQuery.addQueryItem("displayType", "XML");
    formQuery.addQueryItem("sort_asc", "true");
    formQuery.addQueryItem("sort_column", "name");

    // Search critera (required even if blank)
    formQuery.addQueryItem("approvalStatus", "");
    formQuery.addQueryItem("beginDate", "");
    formQuery.addQueryItem("continent", "");
    formQuery.addQueryItem("endDate", "");
    formQuery.addQueryItem("ethnicity", "");
    formQuery.addQueryItem("feature", "");
    formQuery.addQueryItem("featureType", "");
    formQuery.addQueryItem("minFeatureDiameter", "");
    formQuery.addQueryItem("maxFeatureDiameter", "");
    formQuery.addQueryItem("reference", "");
    formQuery.addQueryItem("system", "");

    formQuery.addQueryItem("target", target.toUpper());
    formQuery.addQueryItem("easternLongitude",
                                QString::number(endLon.degrees()));
    formQuery.addQueryItem("westernLongitude",
                                QString::number(startLon.degrees()));
    formQuery.addQueryItem("northernLatitude",
                                 QString::number(endLat.degrees()));
    formQuery.addQueryItem("southernLatitude",
                                 QString::number(startLat.degrees()));

    m_networkMgr->post(*m_request, formQuery.query(QUrl::FullyEncoded).toUtf8());
  }
}
