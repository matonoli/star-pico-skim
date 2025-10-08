// C++ headers
#include <iostream>

//
// Forward declarations
//

class StMaker;
class StChain;
class StPicoDstMaker;

//_________________
void runPicoEASkim(const char *inFileName = "/star/u/matonoli/st_physics_18141040_raw_1000075.picoDst.root")
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
  picoMaker->SetStatus("BTowHit*", 1);
  std::cout << "... done" << std::endl;

  std::cout << "Constructing StPicoEASkimmer with StPicoDstMaker" << std::endl;
  // Example of how to create an instance of the StPicoEASkimmer and initialize
  // it with StPicoDstMaker
  StPicoEASkimmer *anaMaker1 = new StPicoEASkimmer(picoMaker, "oPicoEASkimmer_1.root");
  // Add vertex cut
  anaMaker1->setVtxZ(-40., 40.);
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
  int nEvents2Process = picoMaker->chain()->GetEntries();
  std::cout << " Number of events in files: " << nEvents2Process << std::endl;
  // Also one can set a very large number to process, whyle the special return
  // flag will send when there will be EndOfFile (EOF)

  // Processing events
  for (Int_t iEvent=0; iEvent<nEvents2Process; iEvent++) {
    
    if( iEvent % 10 == 0 ) std::cout << "Macro: working on event: " << iEvent << std::endl;
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
