// STAR headers
#include "St_base/StMessMgr.h"

// StPicoEASkimmer headers
#include "StPicoEASkimmer.h"

// StPicoDstMaker headers
#include "StPicoDstMaker/StPicoDstMaker.h"
// StPicoEvent headers
#include "StPicoEvent/StPicoDst.h"
#include "StPicoEvent/StPicoDstReader.h"
#include "StPicoEvent/StPicoEvent.h"
#include "StPicoEvent/StPicoTrack.h"
#include "StPicoEvent/StPicoBTowHit.h"
#include "StPicoEvent/StPicoBTofPidTraits.h"

// ROOT headers
#include "TChain.h"
#include "TFile.h"
#include "TH1F.h"
#include "TH1D.h"
#include "TH2F.h"

// C++ headers
#include <limits>
#include <iostream>

ClassImp(StPicoEASkimmer)

//________________
StPicoEASkimmer::StPicoEASkimmer(StPicoDstMaker *maker, const char* oFileName)
  : StMaker(), mDebug(false), mOutFileName(oFileName), mOutFile(nullptr),
    mPicoDstMaker(maker), mPicoDstReader(nullptr), mPicoDst(nullptr),
    mEventCounter(0), mIsFromMaker(true) {
  // Constructor

  // Set output file name
  mOutFileName = oFileName;

  // Clean trigger ID collection
  if( !mTriggerId.empty() ) {
    mTriggerId.clear();
  }

  // Set default event cut values
  mVtxZ[0] = -70.; mVtxZ[1] = 70.;
  mVtxR[0] = 0.; mVtxR[1] = 2.;

  // Set default track cut values
  mNHits[0] = 15; mNHits[1] = 90;
  mPt[0] = 0.15; mPt[1] = 10.;
  mEta[0] = -1.; mEta[1] = 1.;
}

//________________
StPicoEASkimmer::~StPicoEASkimmer() {
  // Destructor
  /* empty */
}

//________________
Int_t StPicoEASkimmer::Init() {
  // Initialization of the Maker

  if (mDebug) {
    LOG_INFO << "Initializing StPicoEASkimmer..." << endm;
  }

  // Retrieve PicoDst
  if ( mIsFromMaker ) {
    // Check that StPicoDstMaker exists
    if (mPicoDstMaker) {
      // Retrieve pointer to the StPicoDst structure
      mPicoDst = mPicoDstMaker->picoDst();
    }
    else {
      LOG_ERROR << "No StPicoDstMaker has been found. Terminating." << endm;
      return kStErr;
    }
  } // if ( mIsFromMaker )
  else {
    // Check that StPicoDstReader exists
    if ( mPicoDstReader ) {
      // Retrieve pointer to the StPicoDst structure
      mPicoDst = mPicoDstReader->picoDst();
    }
    else {
      LOG_ERROR << "No StPicoDstMaker has been found. Terminating." << endm;
      return kStErr;
    }
  } // else

  // Check that picoDst exists
  if (!mPicoDst) {
    LOG_ERROR << "No StPicoDst has been provided. Terminating." << endm;
    return kStErr;
  }

  // Create output file
  if (!mOutFile) {
    mOutFile = new TFile(mOutFileName, "recreate");
  }
  else {
    LOG_WARN << "Output file: " << mOutFileName << " already exist!" << endm;
  }

  // Create histograms
  CreateHistograms();

  if (mDebug) {
    LOG_INFO << "StPicoEASkimmer has been initialized" << endm;
  }

  return kStOk;
}

//________________
Int_t StPicoEASkimmer::Finish() {
  // Initialization of the Maker

  if (mDebug) {
    LOG_INFO << "Finishing StPicoEASkimmer..." << endm;
  }

  // Write histograms to the file and then close it
  if (mOutFile) {
    LOG_INFO << "Writing file: " << mOutFileName << endm;
    mOutFile->Write();
    mOutFile->Close();
    LOG_INFO << "\t[DONE]" << endm;
  }
  else {
    LOG_WARN << "Output file does not exist. Nowhere to write!" << endm;
  }

  if (mDebug) {
    LOG_INFO << "StPicoEASkimmer has been initialized" << endm;
  }

  return kStOk;
}

//________________
void StPicoEASkimmer::CreateHistograms() {
  if (mDebug) {
    LOG_INFO << "Creating histograms..." << endm;
  }

  // Event-level QA histograms
  hVtxXvsY = new TH2F("hVtxXvsY", "Primary vertex y vs. x; x (cm); y (cm)", 200, -10., 10., 200, -10., 10.);
  hVtxZ = new TH1F("hVtxZ", "Primary vertex z (TPC); z (cm); Entries", 240, -120., 120.);
  hVtxVpdZ = new TH1F("hVtxVpdZ", "VPD vertex z; z (cm); Entries", 240, -120., 120.);
  hDeltaVz = new TH1F("hDeltaVz", "Delta z (TPC - VPD); #Delta z (cm); Entries", 200, -10., 10.);
  hVtxZvsVpdZ = new TH2F("hVtxZvsVpdZ", "VPD vertex z vs TPC vertex z; z_{VPD} (cm); z_{TPC} (cm)", 240, -120., 120., 240, -120., 120.);

  hRefMult = new TH1F("hRefMult", "Reference multiplicity; RefMult; Entries", 200, 0, 200);
  hGRefMult = new TH1F("hGRefMult", "Global reference multiplicity; gRefMult; Entries", 200, 0, 200);
  hRefMultVsGRefMult = new TH2F("hRefMultVsGRefMult", "RefMult vs gRefMult; gRefMult; RefMult", 200, 0, 200, 200, 0, 200);
  hRefMultVsVz = new TH2F("hRefMultVsVz", "RefMult vs z_{TPC}; z_{TPC} (cm); RefMult", 240, -120., 120., 200, 0, 200);
  hNPrimaries = new TH1F("hNPrimaries", "Number of primary tracks per event; N_{primaries}; Entries", 100, 0, 100);
  hNBTofMatch = new TH1F("hNBTofMatch", "Number of BTOF-matched tracks per event; N_{BTofMatch}; Entries", 100, 0, 100);
  hNBEmcMatch = new TH1F("hNBEmcMatch", "Number of BEMC-matched tracks per event; N_{BEmcMatch}; Entries", 100, 0, 100);
  
  hBBCx = new TH1F("hBBCx", "BBC coincidence rate; BBCx; Entries", 1000, 0, 1e7);
  hZDCx = new TH1F("hZDCx", "ZDC coincidence rate; ZDCx; Entries", 1000, 0, 1e7);
  hRefMultVsBBCx = new TH2F("hRefMultVsBBCx", "RefMult vs BBCx; BBCx; RefMult", 1000, 0, 1e7, 1000, 0, 1000);
  hNPrimariesVsBBCx = new TH2F("hNPrimariesVsBBCx", "N_{primaries} vs BBCx; BBCx; N_{primaries}", 1000, 0, 1e7, 100, 0, 100);
  hNBTofMatchVsBBCx = new TH2F("hNBTofMatchVsBBCx", "N_{BTofMatch} vs BBCx; BBCx; N_{BTofMatch}", 1000, 0, 1e7, 100, 0, 100);
  hRefMultVsZDCx = new TH2F("hRefMultVsZDCx", "RefMult vs ZDCx; ZDCx; RefMult", 1000, 0, 1e7, 1000, 0, 1000);
  hNPrimariesVsZDCx = new TH2F("hNPrimariesVsZDCx", "N_{primaries} vs ZDCx; ZDCx; N_{primaries}", 1000, 0, 1e7, 100, 0, 100);
  hNBTofMatchVsZDCx = new TH2F("hNBTofMatchVsZDCx", "N_{BTofMatch} vs ZDCx; ZDCx; N_{BTofMatch}", 1000, 0, 1e7, 100, 0, 100);

  // Track-Level QA histograms
  hPrimaryPt = new TH1D("hPrimaryPt", "Primary track p_{T}; p_{T} (GeV/c); Entries", 400, 0., 20.);
  hPrimaryEtaVsPhi = new TH2D("hPrimaryEtaVsPhi", "Primary track #eta vs. #phi; #phi (rad); #eta", 360, 0, 2*TMath::Pi(), 300, -1.2, 1.2);
  hPrimaryEtaVsPt = new TH2D("hPrimaryEtaVsPt", "Primary track #eta vs. p_{T}; p_{T} (GeV/c); #eta", 200, 0., 20., 300, -1.2, 1.2);
  hPrimaryPhiVsPt = new TH2D("hPrimaryPhiVsPt", "Primary track #phi vs. p_{T}; p_{T} (GeV/c); #phi (rad)", 200, 0., 20., 360, 0, 2*TMath::Pi());
  hPrimaryNHitsFit = new TH1D("hPrimaryNHitsFit", "Primary track nHitsFit; nHitsFit; Entries", 50, 0, 50);
  hPrimaryNHitsFitVsPt = new TH2D("hPrimaryNHitsFitVsPt", "Primary track nHitsFit vs. p_{T}; p_{T} (GeV/c); nHitsFit", 200, 0., 20., 50, 0, 50);
  hPrimaryNHitsDedx = new TH1D("hPrimaryNHitsDedx", "Primary track nHitsDedx; nHitsDedx; Entries", 50, 0, 50);
  hPrimaryNHitsDedxVsPt = new TH2D("hPrimaryNHitsDedxVsPt", "Primary track nHitsDedx vs. p_{T}; p_{T} (GeV/c); nHitsDedx", 200, 0., 20., 50, 0, 50);
  hPrimaryNHitsFitRatio = new TH1D("hPrimaryNHitsFitRatio", "Primary track nHitsFit/nHitsPoss; nHitsFit/nHitsPoss; Entries", 100, 0., 1.1);
  hPrimaryNHitsFitRatioVsPt = new TH2D("hPrimaryNHitsFitRatioVsPt", "Primary track nHitsFit/nHitsPoss vs. p_{T}; p_{T} (GeV/c); nHitsFit/nHitsPoss", 200, 0., 20., 100, 0., 1.1);
  hPrimaryDCA = new TH1D("hPrimaryDCA", "Primary track DCA; DCA (cm); Entries", 200, 0., 4.);
  hPrimaryDCAVsPt = new TH2D("hPrimaryDCAVsPt", "Primary track DCA vs. p_{T}; p_{T} (GeV/c); DCA (cm)", 200, 0., 20., 200, 0., 4.);

  // TOF QA histograms (
  hPrimaryTofInvBetaVsP = new TH2D("hPrimaryTofInvBetaVsP", "Primary track 1/#beta vs momentum; p (GeV/c); 1/#beta", 200, 0., 20., 200, 0.0, 4.0);
  hPrimaryTofMass2VsP = new TH2D("hPrimaryTofMass2VsP", "Primary track Mass^{2} vs momentum; p (GeV/c); m^{2} (GeV^{2}/c^{4})", 200, 0., 20., 200, 0., 2.);
  hPrimaryTofEtaVsPhi = new TH2D("hPrimaryTofEtaVsPhi", "Primary track #eta vs #phi (TOF-matched); #phi (rad); #eta", 360, 0, 2*TMath::Pi(), 300, -1.2, 1.2);
  hPrimaryTofMatchVsPt = new TH2D("hPrimaryTofMatchVsPt", "Primary track TOF matching flag vs p_{T}; p_{T} (GeV/c); TOF matching flag", 200, 0., 20., 2, -0.5, 1.5);

  // BEMC QA histograms
  hPrimaryBemcE = new TH1D("hPrimaryBemcE", "Primary track matched cluster energy; Energy (GeV); Entries", 500, 0., 50.);
  hPrimaryBemcEPvsPt = new TH2D("hPrimaryBemcEPvsPt", "Primary track p/E vs p_{T}; p_{T} (GeV/c); p/E", 200, 0., 20., 200, 0., 5.);
  hPrimaryBemcDeltaZVsPt = new TH2D("hPrimaryBemcDeltaZVsPt", "Primary track #Delta z vs p_{T}; p_{T} (GeV/c); #Delta z (cm)", 200, 0., 20., 200, -50., 50.);
  hPrimaryBemcDeltaPhiVsPt = new TH2D("hPrimaryBemcDeltaPhiVsPt", "Primary track #Delta #phi vs p_{T}; p_{T} (GeV/c); #Delta #phi (rad)", 200, 0., 20., 200, -0.1, 0.1);
  hPrimaryBemcDeltaZVsDeltaPhi = new TH2D("hPrimaryBemcDeltaZVsDeltaPhi", "Primary track #Delta z vs #Delta #phi; #Delta #phi (rad); #Delta z (cm)", 200, -0.1, 0.1, 200, -50., 50.);
  hPrimaryBemcEtaVsPhi = new TH2D("hPrimaryBemcEtaVsPhi", "Primary track #eta vs #phi (BEMC-matched); #phi (rad); #eta", 360, 0., 2*TMath::Pi(), 300, -1.2, 1.2);

  // PID QA histograms 
  hPrimaryTPCDedxVsP = new TH2D("hPrimaryTPCDedxVsP", "Primary track dE/dx vs momentum; p (GeV/c); dE/dx (keV/cm)", 200, 0., 20., 120, 0., 10.);
  hPrimaryTPCnSigmaPiVsP = new TH2D("hPrimaryTPCnSigmaPiVsP", "Primary track n#sigma_{#pi} vs momentum; p (GeV/c); n#sigma_{#pi}", 200, 0., 15., 200, -10., 10.);
  hPrimaryTPCnSigmaKVsP = new TH2D("hPrimaryTPCnSigmaKVsP", "Primary track n#sigma_{K} vs momentum; p (GeV/c); n#sigma_{K}", 200, 0., 15., 200, -10., 10.);
  hPrimaryTPCnSigmaPVsP = new TH2D("hPrimaryTPCnSigmaPVsP", "Primary track n#sigma_{p} vs momentum; p (GeV/c); n#sigma_{p}", 200, 0., 15., 200, -10., 10.);
  hPrimaryTPCnSigmaEVsP = new TH2D("hPrimaryTPCnSigmaEVsP", "Primary track n#sigma_{e} vs momentum; p (GeV/c); n#sigma_{e}", 200, 0., 15., 200, -10., 10.);

  // Run Dependence histograms
  hBBCxVsRun = new TH2D("hBBCxVsRun", "BBCx vs Run; Run ID; BBCx", 1000, 0, 1000, 100, 0, 1e7);
  hNPrimariesVsRun = new TH2D("hNPrimariesVsRun", "# primary tracks/event vs Run; Run ID; # primary tracks/event", 1000, 0, 1000, 100, 0, 100);
  hNTofMatchedTracksVsRun = new TH2D("hNTofMatchedTracksVsRun", "# TOF-matched tracks/event vs Run; Run ID; # TOF-matched tracks/event", 1000, 0, 1000, 100, 0, 100);
  hDeltaVZVsRun = new TH2D("hDeltaVZVsRun", "#Delta Vz (TPC - VPD) vs Run; Run ID; #Delta Vz (cm)", 1000, 0, 1000, 200, -10., 10.);
  hNHitsFitVsRun = new TH2D("hNHitsFitVsRun", "nHitsFit vs Run; Run ID; nHitsFit", 1000, 0, 1000, 50, 0, 50);
  hNHitsDedxVsRun = new TH2D("hNHitsDedxVsRun", "nHitsDedx vs Run; Run ID; nHitsDedx", 1000, 0, 1000, 50, 0, 50);
  hNHitsFitRatioVsRun = new TH2D("hNHitsFitRatioVsRun", "nHitsFit/nHitsPoss vs Run; Run ID; nHitsFit/nHitsPoss", 1000, 0, 1000, 50, 0., 1.1);
  hDCAVsRun = new TH2D("hDCAVsRun", "DCA vs Run; Run ID; DCA (cm)", 1000, 0, 1000, 50, 0., 4.);
  hDedxVsRun = new TH2D("hDedxVsRun", "dE/dx vs Run; Run ID; dE/dx (keV/cm)", 1000, 0, 1000, 50, 0., 10.);

  // BTOW QA histograms
  hBTowEVsId = new TH2D("hBTowEVsId", "BTOW hit energy vs ID; ID; Energy (GeV)", 4800, 0, 4800, 200, 0., 20.);
  hBTowAdcVsId = new TH2D("hBTowAdcVsId", "BTOW hit ADC vs ID; ID; ADC", 4800, 0, 4800, 100, 0., 100.);
  hBTowPhiVsE = new TH2D("hBTowPhiVsE", "BTOW hit #phi vs Energy; Energy (GeV); #phi (rad)", 200, 0., 20., 360, 0, 2*TMath::Pi());
  hBTowEtaVsE = new TH2D("hBTowEtaVsE", "BTOW hit #eta vs Energy; Energy (GeV); #eta", 200, 0., 20., 300, -1.2, 1.2);
  hBTowEtaVsPhi = new TH2D("hBTowEtaVsPhi", "BTOW hit #eta vs #phi; #phi (rad); #eta", 360, 0, 2*TMath::Pi(), 300, -1.2 , 1.2);

  if (mDebug) {
    LOG_INFO << "All histograms have been created." << endm;
  }
}

//________________
Bool_t StPicoEASkimmer::IsGoodTrigger(StPicoEvent *event) {

  Bool_t isGood = false;
  if ( !mTriggerId.empty()) {
    for (unsigned int iIter=0; iIter<mTriggerId.size(); iIter++) {
      if ( event->isTrigger( mTriggerId.at(iIter) ) ) {
        isGood = true;
        break;
      } // if ( event->isTrigger( mTriggerId.at(iIter) ) )
    } // for (unsigned int iIter=0; iIter<mTriggerId.size(); iIter++)
  } // if ( !mTriggerId.empty())
  else {
    isGood = true;
  } // else

  return isGood;
}

//________________
Bool_t StPicoEASkimmer::EventCut(StPicoEvent *event) {
  return ( event->primaryVertex().Z() >= mVtxZ[0] &&
     event->primaryVertex().Z() <=  mVtxZ[1] &&
     event->primaryVertex().Perp() >= mVtxR[0] &&
     event->primaryVertex().Perp() <= mVtxR[1] &&
     IsGoodTrigger( event ) );
}

//________________
Bool_t StPicoEASkimmer::TrackCut(StPicoTrack *track) {
  return ( track->nHits() >= mNHits[0] &&
     track->nHits() <= mNHits[1] &&
     track->gPt() >= mPt[0] &&
     track->gPt() <= mPt[1] &&
     track->gMom().Eta() >= mEta[0] &&
     track->gMom().Eta() <= mEta[1] );
}

//________________
Int_t StPicoEASkimmer::Make() {

  // Increment event counter
  mEventCounter++;

  // Print event counter
  if ( (mEventCounter % 10000) == 0) {
    // Avoid dereferencing mPicoDstMaker if this maker was constructed with a reader
    if (mPicoDstMaker && mPicoDstMaker->chain()) {
      LOG_INFO << "Working on event: " << mEventCounter << "/" << mPicoDstMaker->chain()->GetEntries() << endm;
    } else {
      LOG_INFO << "Working on event: " << mEventCounter << endm;
    }
  }

  // Check that PicoDst exists
  if ( !mPicoDst ) {
    LOG_ERROR << "No PicoDst has been found. Terminating" << endm;
    return kStErr;
  }

  //
  // The example that shows how to access event information
  //

  // Retrieve pico event
  StPicoEvent *theEvent = mPicoDst->event();
  if ( !theEvent ) {
    LOG_ERROR << "PicoDst does not contain event information. Terminating" << endm;
    return kStErr;
  }

  // Check if event passes event cut (declared and defined in this
  // analysis maker)
  if ( !EventCut(theEvent) ) {
    return kStOk;
  }

  // Fill event histograms
  hVtxXvsY->Fill( theEvent->primaryVertex().X(),
      theEvent->primaryVertex().Y() );
  hVtxZ->Fill( theEvent->primaryVertex().Z() );


  //
  // The example that shows how to access track information
  //

  // Retrieve number of tracks in the event. Make sure that
  // SetStatus("Track*",1) is set to 1. In case of 0 the number
  // of stored tracks will be 0, even if those exist
  unsigned int nTracks = mPicoDst->numberOfTracks();

  // Track loop
  for (unsigned int iTrk=0; iTrk<nTracks; iTrk++) {

    // Retrieve i-th pico track
    StPicoTrack *theTrack = (StPicoTrack*)mPicoDst->track(iTrk);
    // Track must exist
    if (!theTrack) continue;

    // Check if track passes track cut
    if ( !TrackCut(theTrack) ) continue;

    // Fill global track parameters
    hGlobalPt->Fill( theTrack->gPt() );
    hGlobalNHits->Fill( theTrack->nHits() );
    hGlobalEta->Fill( theTrack->gMom().Eta() );

    // Primary track analysis
    if ( !theTrack->isPrimary() ) continue;

    // Fill primary track histograms
    hPrimaryPt->Fill( theTrack->pPt() );
    hPrimaryNHits->Fill( theTrack->nHits() );
    hPrimaryEta->Fill( theTrack->pMom().Eta() );
    hPrimaryDedxVsPt->Fill( theTrack->charge() * theTrack->pPt(),
          theTrack->dEdx() );

    // Accessing TOF PID traits information.
    // One has to remember that TOF information is valid for primary tracks ONLY.
    // For global tracks the path length and time-of-flight have to be
    // recalculated by hands.
    if ( theTrack->isTofTrack() ) {
      StPicoBTofPidTraits *trait =
      (StPicoBTofPidTraits*)mPicoDst->btofPidTraits( theTrack->bTofPidTraitsIndex() );
      if (!trait) continue;

      // Fill primary track TOF information
      hPrimaryInvBetaVsP->Fill( theTrack->charge() * theTrack->pPt(),
            1./trait->btofBeta() );
    } // if ( theTrack->isTofTrack() )
  } // for (Int_t iTrk=0; iTrk<nTracks; iTrk++)

  //
  // The example that shows how to access hit information
  //

  // Get number of BTOW hits (make sure that SetStatus("BTowHit*",1) is set to 1)
  // If it is set to 0, then mPicoDst will return 0 hits all the time
  UInt_t nBTowHits = mPicoDst->numberOfBTowHits();

  // Dummy check, but always good to know that the amount is okay
  if (nBTowHits > 0) {

    // Loop over BTOW hits
    for (UInt_t iHit=0; iHit<nBTowHits; iHit++) {
      // Retrieve i-th BTOW hit
      StPicoBTowHit *btowHit = (StPicoBTowHit*)mPicoDst->btowHit(iHit);
      // Hit must exist
      if (!btowHit) continue;
      // Fill tower ADC
      hBemcTowerAdc->Fill( btowHit->adc() );
    } // for (UInt_t iHit=0; iHit<nBTowHits; iHit++)
  } // if (nBTowHits > 0)

  return kStOk;
}
