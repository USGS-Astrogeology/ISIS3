/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "Isis.h"
#include <QFile>
#include <QList>
#include <QPair>
#include <QDebug>
#include "PvlFlatMap.h"
#include "Resource.h"
#include "Strategy.h"
#include "Cube.h"
#include "GisGeometry.h"
#include "Application.h"
#include "Preference.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlObject.h"
#include "Statistics.h"

using namespace Isis;
using namespace std;

/**
 * @internal
 *   @history 2016-02-29 Tyler Wilson
 *
 *   @TODO:  Need tests for applyToIntersectedGeometry, queryCallback
 *      although the probability is high that the code is being
 *      called in other places in isisminer
 *    @TODO Testing and documentation is needed for LoadMinerStrategies
 *
 *
 */



void printResourceList(const ResourceList &list);
void printMap(const PvlFlatMap &map);
void discardResource(ResourceList &lst,int ix);
void activateResource(ResourceList &lst, int ix);

//Derived class to access protected member functions in Strategy

class DerivedStrategy: public Strategy {

public:


  DerivedStrategy():Strategy() { }

  DerivedStrategy(const PvlObject &definition,const ResourceList &globals)
    :Strategy (definition,globals) { }

  DerivedStrategy(const QString &name, const QString &type):Strategy(name,type) { }

  void setNameType(const QString & name, const QString &type) {
    setName(name);
    setType(type);
  }

  const PvlObject &getDefinitionA() {
    return getDefinition();
  }

  ResourceList getGlobalDefaultsA() {
    return getGlobalDefaults();
  }

  ResourceList getGlobalsA(SharedResource &myGlobals,const ResourceList &globals) {
    return getGlobals(myGlobals,globals);
  }

  PvlFlatMap getDefinitionMapA() {
    return getDefinitionMap();
  }

  void setApplyToDiscardedA() {
    setApplyToDiscarded();
  }

  bool isApplyToDiscardedA() {
    return isApplyToDiscarded();
  }

  void setDoNotApplyToDiscardedA() {
    setDoNotApplyToDiscarded();
  }

  int applyToResourcesA(ResourceList &resources, const ResourceList &globals) {
    return applyToResources(resources,globals);
  }

  unsigned int processedA() {
    return processed();
  }

  void resetProcessedA() {
    return resetProcessed();
  }

  int countActiveA(const ResourceList &resources) {
    return countActive(resources);
  }

  int countDiscardedA(const ResourceList &resources) {
    return countDiscarded(resources);
  }

  ResourceList assetResourceListA(const SharedResource &resource, const QString &name) {
    return assetResourceList(resource,name);
  }
  QString findReplacementA(const QString &target, const ResourceList &globals,
    const int &index = 0, const QString &defValue = "") {
    return findReplacement(target,globals,index,defValue);
  }

  QStringList qualifiersA(const QString &keyspec, const QString &delimiter = "::") {

    return qualifiers(keyspec,delimiter);

  }

  QString scanAndReplaceA(const QString &input, const QString &target,
    const QString &replacement) {
    return scanAndReplace(input,target,replacement);
  }

  QString translateKeywordArgsA(const QString &value, const ResourceList &globals,
    const QString &defValue = "") {
    return translateKeywordArgs(value,globals,defValue);
  }


  QString processArgsA(const QString &value, const QStringList &argKeys,
                      const ResourceList &globals,
                      const QString &defValue = "") {

       return processArgs(value,argKeys,globals,defValue);
  }

  void propagateKeysA(SharedResource &source, SharedResource &target) {
    return propagateKeys(source,target);
  }

  SharedResource compositeA(SharedResource &resourceA, SharedResource &resourceB,
    const QPair<QString, QString> &keySuffix = qMakePair(QString("A"),QString("B"))) {

    return composite(resourceA, resourceB,keySuffix);
  }

  bool importGeometryA(SharedResource &resource,
    const ResourceList &globals) {
    return importGeometry(resource,globals);

  }

  ResourceList activeListA(ResourceList &resource) {
      return activeList(resource);
  }

  void activateListA(ResourceList &resources) {
      activateList(resources);
  }

  void deactivateListA(ResourceList &resources) {
      deactivateList(resources);
  }

  ResourceList copyListA(const ResourceList &resources) {
      return copyList(resources);
  }

  ResourceList cloneListA(const ResourceList &resources,
  const bool &withAssets = false) {
      return cloneList(resources,withAssets);
  }

  int applyToIntersectedGeometryA(ResourceList &resources, GisGeometry &geom,
    const ResourceList &globals) {
    return applyToIntersectedGeometry(resources,geom,globals);
  }

  bool isDebugA() {
    return isDebug();
  }

  bool doShowProgressA() {
    return doShowProgress();
  }

  bool initProgressA(const int &nsteps = 0, const QString &text = "") {
    return initProgress(nsteps,text);
  }

  static void queryCallbackA(void *item, void *userdata) {
    return queryCallback(item, userdata);
  }

  QStringList getObjectListA(const PvlObject &object) {
    return getObjectList(object);
  }

  int apply(ResourceList &resources, const ResourceList &globals) {    
    cout << "Calling apply(ResourceList &resources, const ResourceList &globals)"  << endl;
    return 1;
   }

  int apply(SharedResource &resource, const ResourceList &globals){
    cout << "Calling apply(SharedResource &resource, const ResourceList &globals)"  << endl;
    return 1;
  }

};


void IsisMain() {

  Preference::Preferences(true);
  ResourceList lst;
  ResourceList lstA;
  PvlFlatMap elven1Map, elven2Map,elven3Map;

  elven1Map.add("demon","blue balrog");
  elven1Map.add("demon","green balrog");
  elven1Map.add("demon","ugly balrog");
  elven1Map.add("demon","red balrog");
  elven1Map.add("dark","dur");
  elven1Map.add("shield-wall","thangail");

  elven2Map.add("wolf","draug");
  elven2Map.add("Middle-Earth","Endora");
  elven2Map.add("Sun","Anor1");

  elven3Map.add("Sun","Anor");
  elven3Map.add("daisy","eirien");
  elven3Map.add("pipe-weed","galena");

  SharedResource elven1 = SharedResource(new Resource("Elven Word List 1", elven1Map));
  SharedResource elven2 = SharedResource(new Resource("Elven Word List 2", elven2Map));
  SharedResource elven3 = SharedResource(new Resource("Elven Word List 3", elven3Map));

  SharedResource r1 = SharedResource(new Resource("Resource 1"));
  SharedResource r2 = SharedResource(new Resource("Resource 2"));
  SharedResource r3 = SharedResource(new Resource("Resource 3"));
  SharedResource r4 = SharedResource(new Resource("Resource 4"));
  SharedResource r5 = SharedResource(new Resource("Resource 5"));

  ResourceList R;
  R.append(r1);R.append(r2);R.append(r3);R.append(r4);R.append(r5);

  lst.append(elven1);
  lst.append(elven2);
  lst.append(elven3);
  lstA.append(elven1);
  lstA.append(elven3);

  PvlObject elfDictionary("Elven Dictionary");
  PvlObject emptyDictionary("Empty Dictionary");

  elfDictionary += PvlKeyword("Name", "Elven Dictionary");
  elfDictionary += PvlKeyword("Type", "Dictionary");
  elfDictionary += PvlKeyword("ApplyToDiscarded", "false");
  elfDictionary += PvlKeyword("Debug","true");
  elfDictionary += PvlKeyword("PropagateKeywords","wolf");
  elfDictionary += PvlKeyword("PropagateKeyword","Middle-Earth");


  qDebug() << "************************************************"  << endl;
  qDebug() << "*                Constructors                  *"  << endl;
  qDebug() << "************************************************"  << endl;
  qDebug() << endl;
  qDebug() << "Testing default constructor Strategy()  "  << endl;
  Strategy strat1;
  qDebug() << "Name:         " << strat1.name();
  qDebug() << "Type:         " << strat1.type();
  qDebug() << "Description:  " << strat1.description();

  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;

  qDebug() << "Testing Strategy(const PvlObject &def,const ResourceList &globals) constructor:";
  qDebug() << endl;

  //This should throw an error because we haven't added keywords to emptyDictionary
  try {
      Strategy strat2(emptyDictionary,lst);
      }
  catch(IException &e) {
     qDebug() <<  e.toString();
     }

  Strategy strat3(elfDictionary,lst);
  qDebug() << strat3.name() << endl;

  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;

  qDebug() << "Testing Strategy(const QString &name,const QString &type) constructor:" << endl;

  DerivedStrategy strat4("strat4name","strat4type");
  qDebug() << strat4.name() << endl;
  qDebug() << strat4.type() << endl;

  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
  qDebug() << endl;
  qDebug() << endl;
  qDebug() << "************************************************"  << endl;
  qDebug() << "*              Protected Members               *"  << endl;
  qDebug() << "************************************************"  << endl;
  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
  qDebug() << "%             setName, setType                 %";
  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
  qDebug() << endl;

  DerivedStrategy dstrat1;
  dstrat1.setNameType("Derived Strategy Name","Derived Strategy Type");

  qDebug() << "Name:         " << dstrat1.name();
  qDebug() << "Type:         " << dstrat1.type();

  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;

  DerivedStrategy dstrat4(elfDictionary,lst);

  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
  qDebug() << "%             getGlobalDefaults                %";
  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
  qDebug() << endl;

  ResourceList globalDefaults = dstrat4.getGlobalDefaultsA();

  printResourceList(globalDefaults);
  qDebug() << endl;
  qDebug() << endl;

  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
  qDebug() << "%             getGlobals                       %";
  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
  qDebug() << endl;

  ResourceList globals = dstrat4.getGlobalsA(elven3,globalDefaults);

  printResourceList(globals);
  qDebug() << endl;
  qDebug() << endl;

  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
  qDebug() << "%             assetResourceList                %";
  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
  qDebug() << endl;

  QVariant resources;
  resources.setValue(R);

  elven1->addAsset("R",resources);

  ResourceList elven1Resources =dstrat4.assetResourceListA(elven1,"R");

  for (int i = 0;i < elven1Resources.count(); i++) {
    qDebug() << elven1Resources[i]->name() << endl;
  }

  qDebug() << endl;
  qDebug() << endl;

  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
  qDebug() << "%             getDefinition                    %";
  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
  qDebug() << endl;

  PvlObject o = dstrat4.getDefinitionA();
  qDebug() << o.name() << endl;

  qDebug() << endl;
  qDebug() << endl;


  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
  qDebug() << "%             getDefinitionMap                 %";
  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
  qDebug() << endl;



  PvlFlatMap map = dstrat4.getDefinitionMapA();
  printMap(map);

  qDebug() << "************************************************";
  qDebug() << endl;
  qDebug() << "Testing setApplyToDiscarded(), isApplytToDiscarded(), setDoNotApplyToDiscarded()";
  qDebug() << endl;


  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
  qDebug() << "%             isApplyToDiscarded               %";
  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
  qDebug() << endl;

  qDebug() << "isApplyToDiscarded = ";

  if ( dstrat4.isApplyToDiscardedA() )
    qDebug() << "true" << endl;
  else
    qDebug() << "false" << endl;


  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
  qDebug() << "%             setApplyToDiscarded              %";
  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
  qDebug() << endl;


  qDebug() << "Calling setApplyToDiscarded:  ";


  qDebug() << "isApplyToDiscarded = ";
  if( dstrat4.isApplyToDiscardedA() )
    qDebug() << "true" << endl;
  else
    qDebug() << "false" << endl;


  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
  qDebug() << "%         setDoNotApplyToDiscarded             %";
  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
  qDebug() << endl;


  qDebug() << "Calling setDoNotApplyToDiscarded:  ";

  qDebug() << "isApplyToDiscarded = ";

  if( dstrat4.isApplyToDiscardedA() )
    qDebug() << "true" << endl;
  else
    qDebug() << "false" << endl;


  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
  qDebug() << "%               applyToResources               %";
  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
  qDebug() << endl;



  int resourcesProcessed = dstrat4.applyToResourcesA(lstA,globals);

  //Two resources in lstA are both active
  qDebug() << "Number of resources processed = " << resourcesProcessed << endl;

  discardResource(lstA,0);
  dstrat4.setDoNotApplyToDiscardedA();
  resourcesProcessed = dstrat4.applyToResourcesA(lstA,globals);

  qDebug() << "Number of resources processed (after discarding resource 0) = ";
  qDebug() << resourcesProcessed << endl;

  qDebug() << "Call setApplyToDiscarded:" <<endl;
  dstrat4.setApplyToDiscardedA();
  resourcesProcessed = dstrat4.applyToResourcesA(lstA,globals);
  qDebug() << "Number of resources processed (after discarding resource 0) = ";
  qDebug() << resourcesProcessed << endl;

  dstrat4.setDoNotApplyToDiscardedA();
  activateResource(lstA,0);

  resourcesProcessed = dstrat4.applyToResourcesA(lstA,globals);

  qDebug() << "Number of resources processed (after activiating resource 0) = ";
  qDebug() << resourcesProcessed << endl;



  PvlObject elvenPlantsObj("Botany");
  elvenPlantsObj += PvlKeyword("herb","salab");
  elvenPlantsObj += PvlKeyword("snowdrop","niphredil");
  elvenPlantsObj += PvlKeyword("poplar-tree","tulus");
  elvenPlantsObj += PvlKeyword("poplar-tree1","tulus");
  elvenPlantsObj += PvlKeyword("pipe-weed","galenas");

  SharedResource elven4 = SharedResource(new Resource("Sindarin Plant Names",elvenPlantsObj));

  QVariant elfPlants(elven4);


  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
  qDebug() << "%           processed/resetProcessed           %";
  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
  qDebug() << endl;



  qDebug() << "************************************************";
  qDebug() << endl;
  qDebug() << "Testing processed/resetProcessed:  " << endl;

  qDebug() << "Processed = " << dstrat4.processedA() << endl;

  qDebug() << "Resetting Processed:  " << endl;
  dstrat4.resetProcessedA();

  qDebug() << "Processed = " << dstrat4.processedA() << endl;


  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
  qDebug() << "%                  countActive                 %";
  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
  qDebug() << endl;

  qDebug() << "Testing countActive/countDiscard:  " << endl;

  qDebug() << "Active Resources in ResourseList lst:"  << dstrat4.countActiveA(lst) << endl;

  qDebug() << "Discarded Resources in ResourceList lst:"  << dstrat4.countDiscardedA(lst) << endl;

  qDebug() << "Discarding the first resource in ResourceList lst:" << endl;

  discardResource(lst,0);

  qDebug() << "Discarded Resources in ResourceList lst:"  << dstrat4.countDiscardedA(lst) << endl;

  qDebug() << "Active Resources in ResourceList lst:"  << dstrat4.countActiveA(lst) << endl;

  activateResource(lst,0);

  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
  qDebug() << "%             findreplacement                  %";
  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
  qDebug() << endl;


  qDebug() << "Searching for elvish word for demon (with default args):  ";
  qDebug() << dstrat4.findReplacementA("demon",lst)  << endl;

  qDebug() <<"Searching for value not in the ResourceList:  "<< endl;

  QString searchKey = "fluffy bunny";
  QString failMsg = "Could not find "+searchKey;
  qDebug() << dstrat4.findReplacementA(searchKey,lst,0,failMsg)  << endl;


  qDebug() << "Searching for the 100th demon (which is not in lst:  "<< endl;
  qDebug() << dstrat4.findReplacementA(searchKey,lst,100,"100th demon not in lst")  << endl;

  qDebug() << endl;
  qDebug() << endl;


  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
  qDebug() << "%                qualifiers                    %";
  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
  qDebug() << endl;


  QString elvishWordsStartingWithA  = "Aaye,Aelin,Adan,Adanedhel,Aduial,Aglarond";
  QString elvishWordsStartingWithA1  = "Aaye::Aelin::Adan::Adanedhel::Aduial::Aglarond";


  QStringList aWords = dstrat4.qualifiersA(elvishWordsStartingWithA,",");

  for (int i = 0; i < aWords.count(); i++ ) {
    qDebug() << aWords[i] << endl;
  }


  qDebug() << endl;
  qDebug() << "Testing qualifiers with default delimiter (::):  " << endl;


  QStringList aWords1 = dstrat4.qualifiersA(elvishWordsStartingWithA1);

  for (int i = 0; i < aWords1.count(); i++ ) {
    qDebug() << aWords1[i] << endl;
  }


  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
  qDebug() << "%               scanAndReplace                 %";
  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
  qDebug() << endl;

  QString originalSentence("Balrogs require much fnord love and fnord attention.");
  QString fixedSentence = dstrat4.scanAndReplaceA(originalSentence,"fnord","");

  qDebug() << "Original sentence:  "  << originalSentence << endl;
  qDebug() << "Fixed sentence:  "  << fixedSentence << endl;


  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
  qDebug() << "%             translateKeywordArgs             %";
  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
  qDebug() << endl;


  QString modifiedKeyword = dstrat4.translateKeywordArgsA("shield-wall",lst,"blah");

  qDebug() << modifiedKeyword << endl;


  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
  qDebug() << "%                 processArgs                  %";
  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
  qDebug() << endl;

  QStringList argKeys;

  argKeys << "demon" << "dark"  << "shield-wall";

  qDebug() << dstrat4.processArgsA("balrog",argKeys,lst,"default resource") << endl;


  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
  qDebug() << "%                propagateKeys                 %";
  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
  qDebug() << endl;


  qDebug() << "Propagating keys from Shared Resource elven2 -> elven3"  << endl;
  qDebug() << "elven3 keys before propagation:" << endl;

  printMap(elven3->keys());

  dstrat4.propagateKeysA(elven2,elven3);
  qDebug() << endl;
  qDebug() << "elven3 keys after propagation:" << endl;


  printMap(elven3->keys());

  qDebug() << endl;
  qDebug() << endl;


  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
  qDebug() << "%    activeList/deactivateList/activateList    %";
  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
  qDebug() << endl;

  ResourceList l1 = dstrat4.activeListA(lst);
  printResourceList(l1);
  qDebug() << "Number of active resources in DerivedStrategy dstrat4 = ";
  qDebug() << l1.count() << endl;

  qDebug() << "************************************************" << endl;
  qDebug() << "Testing deactivateList" << endl;

  dstrat4.deactivateListA(lst);
  qDebug() << "Number of active resources in DerivedStrategy dstrat4 = ";
  qDebug() << dstrat4.countActiveA(lst) << endl;


  qDebug() << "************************************************" << endl;
  qDebug() << "Testing activateList" << endl;
  dstrat4.activateListA(lst);
  ResourceList l2 = dstrat4.activeListA(lst);
  qDebug() <<"Number of active resources = " << l2.count() << endl;

  qDebug() << "************************************************" << endl;
  qDebug() << "Deactivating Resource 0 in ResourceList lst:" << endl;
  discardResource(lst,0);
  ResourceList l3 = dstrat4.activeListA(lst);
  qDebug() <<"Number of active resources = " << l3.count() << endl;
  activateResource(lst,0);

  qDebug() << endl;
  qDebug() << endl;

  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
  qDebug() << "%                  copyList                    %";
  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
  qDebug() << endl;

  qDebug() << "Testing copyList (globals -> l5)" << endl;
  ResourceList l5 = dstrat4.copyListA(globals);
  printResourceList(l5);
  qDebug() << "Decativating l5 Resources (global resources are active)" << endl;
  dstrat4.deactivateListA(l5);

  qDebug() <<"Number of active resources in l5 = " << dstrat4.countActiveA(l5) << endl;
  qDebug() <<"Number of active resources in global = " << dstrat4.countActiveA(globals) << endl;
  qDebug() << endl;
  qDebug() << endl;

  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
  qDebug() << "%                  cloneList                   %";
  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
  qDebug() << endl;


  qDebug() << "Testing cloneList (globals -> l6)" << endl;
  ResourceList l6 = dstrat4.cloneListA(globals);
  printResourceList(l6);


  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
  qDebug() << "%                  isDebug                     %";
  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
  qDebug() << endl;

  qDebug() << "Testing isDebug():" << endl;
  qDebug() << "isDebug() = " << dstrat4.isDebugA() << endl;


  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
  qDebug() << "%                doShowProgress                %";
  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
  qDebug() << endl;

  qDebug() << "Testing doShowProgress():" << endl;
  qDebug() << "doShowProgress() = "<< dstrat4.doShowProgressA() << endl;

  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
  qDebug() << "%                initProgress                  %";
  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
  qDebug() << endl;

  qDebug() << "Testing initProgress():" << endl;
  qDebug() << "Calling: initProgress() =  " << dstrat4.initProgressA() << endl;
  qDebug() << "Calling: initProgress(2,\"some text\") = ";
  qDebug() <<  dstrat4.initProgressA(2,"some text")<< endl;


  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
  qDebug() << "%                  composite                   %";
  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
  qDebug() << endl;
  qDebug() << endl;

  qDebug() << "Testing composite(...):" << endl;
  SharedResource shared = dstrat4.compositeA(elven2,elven3,qMakePair(QString("A"),QString("B")));
  PvlFlatMap mp = shared->keys();
  printMap(mp);
  qDebug() << endl;
  qDebug() << endl;


  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
  qDebug() << "%                 importGeometry               %";
  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
  qDebug() << endl;
  qDebug() << endl;

  //This call will fail
  dstrat4.importGeometryA(elven1,lst);

  PvlFlatMap line1Map,line2Map,line3Map;

  line1Map.add("GisGeometry","LINESTRING(1.0 22.5, 90.0 65.0)");
  line2Map.add("GisGeometry","LINESTRING(1.0 22.5, 50.0 65.0)");
  line3Map.add("GisGeometry","LINESTRING(1.0 22.5, 20.0 65.0)");

  SharedResource line1 = SharedResource(new Resource("Line 1", line1Map));
  SharedResource line2 = SharedResource(new Resource("Line 2", line2Map));
  SharedResource line3 = SharedResource(new Resource("Line 3", line3Map));

  ResourceList lines;
  lines.append(line1);
  lines.append(line2);
  lines.append(line3);

  PvlObject Geometry("Geom Object");

  Geometry += PvlKeyword("Type","Intersect");
  Geometry += PvlKeyword("Name","H5");
  Geometry += PvlKeyword("GisType","WKT");
  Geometry += PvlKeyword("GisGeometry","LINESTRING(0.0 22.5, 90.0 65.0)");
  Geometry += PvlKeyword("BoundingBox","True");
  DerivedStrategy geoms(Geometry,lines);

  //This call succeeds
  qDebug() << "importGeometry = ";
  qDebug() << geoms.importGeometryA(line2,lines);
  qDebug() << Qt::endl;
  qDebug() << Qt::endl;


  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << Qt::endl;
  qDebug() << "%             repairInvalidGeometry            %";
  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << Qt::endl;
  qDebug() << Qt::endl;
  qDebug() << Qt::endl;

  SharedResource poly1 = SharedResource(new Resource("Polygon 1"));

  ResourceList polygons;
  polygons.append(poly1);

  PvlObject PolygonGeometry("Geom Object");
  PolygonGeometry += PvlKeyword("Type","Intersect");
  PolygonGeometry += PvlKeyword("Name","H5");
  PolygonGeometry += PvlKeyword("GisType","WKT");
  PolygonGeometry += PvlKeyword("GisGeometry","MULTIPOLYGON(((0.0 0.0, 50.0 0.0, 50.0 50.0, 0.0 50.0, 20.0 -10.0, 0.0 0.0)))");
  PolygonGeometry += PvlKeyword("BoundingBox","True");
  PolygonGeometry += PvlKeyword("RepairInvalidGeometry","True");
  DerivedStrategy polygeomsRepair(PolygonGeometry,polygons);

  // The input MULTIPOLYGON is self-intersecting. This call succeeds because
  // RepairInvalidGeometry is True and the repair is successful.
  qDebug() << "importPolygonGeometry (Repair self-intersection on) ="
           << polygeomsRepair.importGeometryA(poly1,polygons) << Qt::endl;

  // Rerun the same test with repair set to false
  PolygonGeometry.addKeyword(PvlKeyword("RepairInvalidGeometry", "False"), Pvl::Replace);
  DerivedStrategy polygeomsNoRepair(PolygonGeometry,polygons);
  qDebug() << "importPolygonGeometry (Repair self-intersection off) ="
           << polygeomsNoRepair.importGeometryA(poly1,polygons);
  qDebug() << Qt::endl;
  qDebug() << Qt::endl;
 

  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << Qt::endl;
  qDebug() << "%             invalidGeometryAction            %";
  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << Qt::endl;
  qDebug() << Qt::endl;
  qDebug() << Qt::endl;

  // confirm Resource from above invalid geometry call has been disabled
  // (Default behavior) as a result of the importGeometry failure   
  qDebug() << "  invalidGeometryAction = disable ";
  qDebug() << "invalidGeometryDisabled =" << poly1->isDiscarded() << Qt::endl;;

  // Rerun the same test with InvalidGeometryAction set to error
  PolygonGeometry.addKeyword(PvlKeyword("InvalidGeometryAction", "Error"));
  DerivedStrategy polygeomsNoRepairActionError(PolygonGeometry,polygons);
  qDebug() << "  invalidGeometryAction = error ";

  try {
    qDebug() << polygeomsNoRepairActionError.importGeometryA(poly1,polygons);
  }
  catch(IException &e){
    qDebug() << e.toString() << Qt::endl;
  }

  // Reactivate polygon and rerun test with InvalidGeometryAction
  // set to Continue. Here, the import geometry action fails, but
  // because InvalidGeometryAction is set to Continue, the polygon
  // resource is NOT disabled.
  poly1->activate();
  PolygonGeometry.addKeyword(PvlKeyword("InvalidGeometryAction", "Continue"), Pvl::Replace);
  DerivedStrategy polygeomsNoRepairActionContinue(PolygonGeometry,polygons);
  qDebug() << "  invalidGeometryAction = continue ";
  qDebug() << "importPolygonGeometry (Repair off; Action continue) ="
           << polygeomsNoRepairActionContinue.importGeometryA(poly1,polygons);
  qDebug() << "invalidGeometryDisabled =" << poly1->isDiscarded() << Qt::endl;
  qDebug() << Qt::endl;


  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << Qt::endl;
  qDebug() << "%                  getObjectList               %";
  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
  qDebug() << endl;

  PvlObject obj("O");
  PvlObject obj1("O1");
  PvlObject obj2("O2");
  PvlObject obj3("O3");
  obj.addObject(obj1);
  obj.addObject(obj2);
  obj.addObject(obj3);

  QStringList objList = dstrat4.getObjectListA(obj);

  for (int i = 0; i < objList.count(); i++ )
    qDebug() << objList[i] << endl;

  qDebug() << endl;
  qDebug() << endl;


  //This test needs to be added to.

  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
  qDebug() << "%         applyToIntersectedGeometry           %";
  qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
  qDebug() << endl;


  GisGeometry emptyGeom;

  //This call will fail
  try {
  qDebug() << dstrat4.applyToIntersectedGeometryA(lst, emptyGeom, globals);
  }
  catch(IException &e){

  qDebug() << e.toString() << endl;

  }



}//end IsisMain()


//Helper functions


/**
 * @brief Activates a resource in a ResourceList
 *
 *
 * @history 2016-03-08 Tyler Wilson
 *
 * @param lst The ResourceList
 *
 * @return ix The index (starting at 0) of the resource in the list
 *  to activate.
 */

void activateResource(ResourceList &lst, int ix){
    lst[ix]->activate();
}



/**
 * @brief Discards a resource in a ResourceList
 *
 *
 * @history 2016-03-08 Tyler Wilson
 *
 * @param lst The ResourceList
 *
 * @return ix The index (starting at 0) of the resource in the list
 *  to discard.
 */

void discardResource(ResourceList &lst,int ix){
    lst[ix]->discard();
}


/**
 * @brief Displays the values of a PvlFlatMap
 *
 *
 * @history 2016-03-08 Tyler Wilson
 *
 * @param map The PvlFlatMap to be displayed.
 *
 */


void printMap(const PvlFlatMap &map){
    PvlFlatMap::ConstPvlFlatMapIterator iter;
    for (iter=map.constBegin(); iter != map.constEnd(); iter++) {
        cout << "\t" << iter.value() << endl;
    }
}

/**
 * @brief Displays the names of shared resources
 *  in a ResourceList.
 *
 * @history 2016-03-08 Tyler Wilson
 *
 * @param list The ResourceList to be displayed.
 *
 */


void printResourceList(const ResourceList &list){
    for (int i = 0; i < list.count(); i++)
       cout << list[i] ->name() << endl;
}
