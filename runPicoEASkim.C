// C++ headers
#include <iostream>

//
// Forward declarations
//

class StMaker;
class StChain;
class StPicoDstMaker;

//_________________
// Can be ran as
// root4star -q -l runPicoEASkim.C\(\"/star/u/matonoli/st_physics_18141040_raw_1000075.picoDst.root\",\"tmp.root\",200\)
void runPicoEASkim(const char *inFileName = "/star/u/matonoli/st_physics_18141040_raw_1000075.picoDst.root",
                   const char *outFileName = "oPicoEASkimmer_1.root",
                   int maxEvents = -1)
{

  std::cout << "Lets run the StPicoEASkimmer." << std::endl;
  // Load all the STAR libraries
  gROOT->LoadMacro("$STAR/StRoot/StMuDSTMaker/COMMON/macros/loadSharedLibraries.C");
  loadSharedLibraries();

  // Load specific libraries
  gSystem->Load("StPicoEvent");
  gSystem->Load("StPicoDstMaker");
  gSystem->Load("StPicoEASkimmer");

  // Create new chain
  StChain *chain = new StChain();

  std::cout << "Creating StPicoDstMaker to read and pass file list" << std::endl;
  // Read via StPicoDstMaker
  // I/O mode: write=1, read=2; input file (or list of files); name
  StPicoDstMaker* picoMaker = new StPicoDstMaker(2, inFileName, "picoDst");
  // Set specific branches ON/OFF
  picoMaker->SetStatus("*", 0);
  picoMaker->SetStatus("Event*", 1);
  picoMaker->SetStatus("Track*", 1);
  picoMaker->SetStatus("BTofPidTraits*", 1);
  picoMaker->SetStatus("EmcTrigger*", 1);
  picoMaker->SetStatus("EmcPidTraits*", 1);
  picoMaker->SetStatus("BTowHit*", 1);
  std::cout << "... done" << std::endl;

  std::cout << "Constructing StPicoEASkimmer with StPicoDstMaker" << std::endl;
  // Example of how to create an instance of the StPicoEASkimmer and initialize
  // it with StPicoDstMaker. Use the provided output filename.
  StPicoEASkimmer *anaMaker1 = new StPicoEASkimmer(picoMaker, outFileName);
  // Configure allowed triggers (documented mapping)
  // Mapping: label -> trigger id(s) (some labels include both DAQ id and trigger bit)
  // BHT1*VPD100: 570204, 29
  // BHT1*VPD30: 570214
  // BHT2*BBCMB: 570205, 570215, 30
  // BHT3: 570201, 16
  // VPDMB100: 570008
  // VPDMB30: 570001, 24
  // VPDMB-novtx: 570004, 55
  // zerobias: 9300
  // TofHighMult: 37

  // Older ROOT interpreters (CINT) may not support C++11 initializer_list syntax.
  // Use a plain C array and a simple loop which is compatible with interpreted macros.
  unsigned int allowedTriggersArr[] = {
    570204, 29, 570214, 570205, 570215, 30, 570201, 16,
    570008, 570001, 24, 570004, 55, 9300, 37
  };
  const int nAllowedTriggers = sizeof(allowedTriggersArr) / sizeof(allowedTriggersArr[0]);
  for (int i = 0; i < nAllowedTriggers; ++i) {
    anaMaker1->addTriggerId(allowedTriggersArr[i]);
  }

  // Calculate runIndex map from a runlist text file
  anaMaker1->LoadRunIndexMap("runlist2017.txt");


  // =============================
  // Cut configuration block (QA and TTree/skimming)
  // This block centralizes all cut definitions so it's easy to adjust
  // selection without touching the maker implementation. These calls
  // are plain function calls and fully compatible with ROOT5/CINT.
  //
  // Section 1: QA cuts (used for histograms and general QA)
  //  - events: vtxZ, vtxR
  //  - tracks: nHitsFit, nHitsRatio (nHitsFit/nHitsPoss), pT, eta
  anaMaker1->setVtxZ(-120., 120.);    // TPC primary vertex z-range (cm)
  anaMaker1->setVtxR(0., 3.);         // primary vertex radial cut (cm)
  anaMaker1->setNHits(15, 90);        // nHitsFit range (min,max)
  anaMaker1->setNHitsRatio(0.0, 1.1);// nHitsFit/nHitsPoss (min,max)
  anaMaker1->setPt(0.15, 50.0);       // track pT (GeV/c)
  anaMaker1->setEta(-1.2, 1.2);       // track pseudorapidity

  // Section 2: Tree-level (skimming) cuts
  // These cuts are applied to decide which events/tracks are written
  // into the compact TTree. They are intentionally separate so you can
  // have looser QA but stricter skim criteria.
  // Event-level tree cuts
  anaMaker1->setTreeVtxZ(-70., 70.);      // TPC vtx z (cm)
  anaMaker1->setTreeVtxR(0., 2.);          // vtx radial (cm)
  anaMaker1->setTreeDeltaVz(-5., 5.);   // (TPC vtx z - VPD vtx z) (cm)
  anaMaker1->setTreeVtxVpdZ(-100., 100.);  // VPD vertex z (wide by default)
  anaMaker1->setTreeNPrimariesMin(1);     // minimum number of primary tracks

  // Track-level tree cuts
  anaMaker1->setTreeNHits(15, 90);        // stricter nHitsFit for tree
  anaMaker1->setTreeNHitsRatio(0.51, 1.1); // stricter hits ratio
  anaMaker1->setTreeNHitsDedx(10, 90);    // min nHitsDedx for dE/dx
  anaMaker1->setTreePt(0.2, 50.0);        // pT for tracks stored in tree
  anaMaker1->setTreeEta(-1.1, 1.1);       // eta for tracks stored in tree
  anaMaker1->setTreeDCA(0., 2.0);         // DCA cut (cm)

  std::cout << "... done" << std::endl;

  std::cout << "Initializing chain" << std::endl;
  // Check that all maker has been successfully initialized
  if( chain->Init() == kStErr ){ 
    std::cout << "Error during the chain initializtion. Exit. " << std::endl;
    return;
  }
  std::cout << "... done" << std::endl;


  std::cout << "Lets process data." << std::endl;
  // Retrieve number of events picoDst files
  int nEvents2Process = (int)picoMaker->chain()->GetEntries();
  std::cout << " Number of events in files: " << nEvents2Process << std::endl;
  // If the user supplied a positive maxEvents, use that as the upper limit.
  if (maxEvents > 0 && maxEvents < nEvents2Process) {
    std::cout << " Limiting processing to " << maxEvents << " events as requested." << std::endl;
    nEvents2Process = maxEvents;
  }
  // Also one can set a very large number to process, while the special return
  // flag will send when there will be EndOfFile (EOF)

  // Processing events
  for (Int_t iEvent=0; iEvent<nEvents2Process; iEvent++) {
    
    if( iEvent % 1000 == 0 ) std::cout << "Macro: working on event: " << iEvent << std::endl;
    chain->Clear();

    // Check return code
    int iret = chain->Make();
    // Quit event processing if return code is not 0
    if (iret) { std::cout << "Bad return code!" << iret << endl; break; }
  } // for (Int_t iEvent=0; iEvent<nEvents2Process; iEvent++)
  std::cout << "Data have been processed." << std::endl;

  std::cout << "Finalizing chain" << std::endl;
  // Finalize all makers in chain
  chain->Finish();
  delete chain;

  std::cout << "... done" << std::endl;
  std::cout << "Analysis has been finished." << std::endl;
}
