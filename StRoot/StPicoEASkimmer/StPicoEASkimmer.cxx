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
#include "StPicoEvent/StPicoBTofPidTraits.h"
#include "StPicoEvent/StPicoBEmcPidTraits.h"
#include "StPicoEvent/StPicoBTowHit.h"
#include "StPicoEvent/StPicoEmcTrigger.h"

// ROOT headers
#include "TChain.h"
#include "TFile.h"
#include "TH1F.h"
#include "TH1D.h"
#include "TH2F.h"
#include "TVector3.h"
#include "TMath.h"

// C++ headers
#include <limits>
#include <iostream>
#include <algorithm>

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

  // Clean the run indexing map. Note that the map definition
  // is to be called from the steering macro
  mRunIndexMap.clear();

  // Set default event cut values
  mCutVtxZ[0] = -70.; mCutVtxZ[1] = 70.;
  mCutVtxR[0] = 0.; mCutVtxR[1] = 2.;

  // Default QA hits ratio (nHitsFit / nHitsPoss)
  mCutNHitsRatio[0] = 0.0; mCutNHitsRatio[1] = 1.1;

  // Set default track cut values
  mCutNHits[0] = 15; mCutNHits[1] = 90;
  mCutPt[0] = 0.15; mCutPt[1] = 10.;
  mCutEta[0] = -1.2; mCutEta[1] = 1.2;

  // Default tree-level cuts (skimming)
  mTreeCutVtxZ[0] = -70.; mTreeCutVtxZ[1] = 70.;
  mTreeCutVtxR[0] = 0.; mTreeCutVtxR[1] = 2.;
  mTreeCutDeltaVz[0] = -30.; mTreeCutDeltaVz[1] = 30.;
  mTreeCutVtxVpdZ[0] = -200.; mTreeCutVtxVpdZ[1] = 200.;
  mTreeCutNPrimariesMin = 0;

  mTreeCutNHits[0] = 15; mTreeCutNHits[1] = 90;
  mTreeCutNHitsRatio[0] = 0.0; mTreeCutNHitsRatio[1] = 1.1;
  mTreeCutNHitsDedx[0] = 0; mTreeCutNHitsDedx[1] = 90;
  mTreeCutPt[0] = 0.15; mTreeCutPt[1] = 10.;
  mTreeCutEta[0] = -1.2; mTreeCutEta[1] = 1.2;
  mTreeCutDCA[0] = 0.; mTreeCutDCA[1] = 1000.;
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

  // Book TTree and branches
  CreateEATree();

  if (mDebug)
  {
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

  // Counter histograms
  hEventCounter = new TH1F("hEventCounter", "Event counter; counter; Events", 10, -0.5, 9.5);
  hTrackCounter = new TH1F("hTrackCounter", "Track counter; counter; Tracks", 10, -0.5, 9.5);

  // Event-level QA histograms
  hVtxXVsY = new TH2F("hVtxXVsY", "Primary vertex y vs. x; x (cm); y (cm)", 200, -1., 1., 200, -1., 1.);
  hVtxZ = new TH1F("hVtxZ", "Primary vertex z (TPC); z (cm); Entries", 400, -200., 200.);
  hVtxVpdZ = new TH1F("hVtxVpdZ", "VPD vertex z; z (cm); Entries", 400, -200., 200.);
  hDeltaVz = new TH1F("hDeltaVz", "Delta z (TPC - VPD); #Delta z (cm); Entries", 200, -10., 10.);
  hVtxZVsVpdZ = new TH2F("hVtxZVsVpdZ", "VPD vertex z vs TPC vertex z; z_{VPD} (cm); z_{TPC} (cm)", 400, -200., 200., 400, -200., 200.);
  hVtxRanking = new TH1F("hVtxRank", "Primary vertex ranking; ranking; Entries", 200, 0, 1e7);
  hVtxErrorXY = new TH1F("hVtxErrorXY", "Primary vertex error in xy; error (cm); Entries", 100, 0., 0.5);
  hVtxErrorZ = new TH1F("hVtxErrorZ", "Primary vertex error in z; error (cm); Entries", 100, 0., 0.5);

  hRefMult = new TH1F("hRefMult", "Reference multiplicity; RefMult; Entries", 100, 0, 100);
  hGRefMult = new TH1F("hGRefMult", "Global reference multiplicity; gRefMult; Entries", 100, 0, 100);
  hRefMultVsGRefMult = new TH2F("hRefMultVsGRefMult", "RefMult vs gRefMult; gRefMult; RefMult", 100, 0, 100, 100, 0, 100);
  hRefMultVsVz = new TH2F("hRefMultVsVz", "RefMult vs z_{TPC}; z_{TPC} (cm); RefMult", 240, -120., 120., 100, 0, 100);
  hNPrimaries = new TH1F("hNPrimaries", "Number of primary tracks per event; N_{primaries}; Entries", 100, 0, 100);
  hNBTofMatch = new TH1F("hNBTofMatch", "Number of BTOF-matched tracks per event; N_{BTofMatch}; Entries", 100, 0, 100);
  hNBEmcMatch = new TH1F("hNBEmcMatch", "Number of BEMC-matched tracks per event; N_{BEmcMatch}; Entries", 100, 0, 100);
  
  hBBCx = new TH1F("hBBCx", "BBC coincidence rate; BBCx; Entries", 1000, 0, 1e7);
  hZDCx = new TH1F("hZDCx", "ZDC coincidence rate; ZDCx; Entries", 1000, 0, 4e6);
  hVtxErrorXYVsBBCx = new TH2F("hVtxErrorXYVsBBCx", "Primary vertex error in xy vs BBCx; BBCx; error (cm)", 1000, 0, 1e7, 100, 0., 0.5);
  hVtxErrorZVsBBCx = new TH2F("hVtxErrorZVsBBCx", "Primary vertex error in z vs BBCx; BBCx; error (cm)", 1000, 0, 1e7, 100, 0., 0.5);
  hRefMultVsBBCx = new TH2F("hRefMultVsBBCx", "RefMult vs BBCx; BBCx; RefMult", 1000, 0, 1e7, 100, 0, 100);
  hNPrimariesVsBBCx = new TH2F("hNPrimariesVsBBCx", "N_{primaries} vs BBCx; BBCx; N_{primaries}", 1000, 0, 1e7, 100, 0, 100);
  hNBTofMatchVsBBCx = new TH2F("hNBTofMatchVsBBCx", "N_{BTofMatch} vs BBCx; BBCx; N_{BTofMatch}", 1000, 0, 1e7, 100, 0, 100);
  hRefMultVsZDCx = new TH2F("hRefMultVsZDCx", "RefMult vs ZDCx; ZDCx; RefMult", 1000, 0, 4e6, 100, 0, 100);
  hNPrimariesVsZDCx = new TH2F("hNPrimariesVsZDCx", "N_{primaries} vs ZDCx; ZDCx; N_{primaries}", 1000, 0, 4e6, 100, 0, 100);
  hNBTofMatchVsZDCx = new TH2F("hNBTofMatchVsZDCx", "N_{BTofMatch} vs ZDCx; ZDCx; N_{BTofMatch}", 1000, 0, 4e6, 100, 0, 100);

  // Track-Level QA histograms
  hPrimaryPt = new TH1D("hPrimaryPt", "Primary track p_{T}; p_{T} (GeV/c); Entries", 500, 0., 20.);
  hPrimaryEta = new TH1D("hPrimaryEta", "Primary track #eta; #eta; Entries", 300, -1.2, 1.2);
  hPrimaryPhi = new TH1D("hPrimaryPhi", "Primary track #phi; #phi (rad); Entries", 360, -TMath::Pi(), TMath::Pi());
  hPrimaryEtaVsPhi = new TH2D("hPrimaryEtaVsPhi", "Primary track #eta vs. #phi; #phi (rad); #eta", 360, -TMath::Pi(), TMath::Pi(), 300, -1.2, 1.2);
  hPrimaryEtaVsPt = new TH2D("hPrimaryEtaVsPt", "Primary track #eta vs. p_{T}; p_{T} (GeV/c); #eta", 200, 0., 20., 300, -1.2, 1.2);
  hPrimaryPhiVsPt = new TH2D("hPrimaryPhiVsPt", "Primary track #phi vs. p_{T}; p_{T} (GeV/c); #phi (rad)", 200, 0., 20., 360, -TMath::Pi(), TMath::Pi());
  hPrimaryNHitsFit = new TH1D("hPrimaryNHitsFit", "Primary track nHitsFit; nHitsFit; Entries", 50, 0, 50);
  hPrimaryNHitsFitVsPt = new TH2D("hPrimaryNHitsFitVsPt", "Primary track nHitsFit vs. p_{T}; p_{T} (GeV/c); nHitsFit", 200, 0., 20., 50, 0, 50);
  hPrimaryNHitsDedx = new TH1D("hPrimaryNHitsDedx", "Primary track nHitsDedx; nHitsDedx; Entries", 50, 0, 50);
  hPrimaryNHitsDedxVsPt = new TH2D("hPrimaryNHitsDedxVsPt", "Primary track nHitsDedx vs. p_{T}; p_{T} (GeV/c); nHitsDedx", 200, 0., 20., 50, 0, 50);
  hPrimaryNHitsFitRatio = new TH1D("hPrimaryNHitsFitRatio", "Primary track nHitsFit/nHitsPoss; nHitsFit/nHitsPoss; Entries", 100, 0., 1.1);
  hPrimaryNHitsFitRatioVsPt = new TH2D("hPrimaryNHitsFitRatioVsPt", "Primary track nHitsFit/nHitsPoss vs. p_{T}; p_{T} (GeV/c); nHitsFit/nHitsPoss", 200, 0., 20., 100, 0., 1.1);
  hPrimaryChi2 = new TH1D("hPrimaryChi2", "Primary track chi^{2}/ndf; chi^{2}/ndf; Entries", 100, 0., 10.);
  hPrimaryChi2VsPt = new TH2D("hPrimaryChi2VsPt", "Primary track chi^{2}/ndf vs. p_{T}; p_{T} (GeV/c); chi^{2}/ndf", 200, 0., 20., 100, 0., 10.);
  hPrimaryDCA = new TH1D("hPrimaryDCA", "Primary track DCA; DCA (cm); Entries", 200, 0., 4.);
  hPrimaryDCAVsPt = new TH2D("hPrimaryDCAVsPt", "Primary track DCA vs. p_{T}; p_{T} (GeV/c); DCA (cm)", 200, 0., 20., 200, 0., 4.);
  hPrimaryDCAxy = new TH1D("hPrimaryDCAxy", "Primary track DCA_{xy}; DCA_{xy} (cm); Entries", 200, 0., 4.);
  hPrimaryDCAxyVsPt = new TH2D("hPrimaryDCAxyVsPt", "Primary track DCA_{xy} vs. p_{T}; p_{T} (GeV/c); DCA_{xy} (cm)", 200, 0., 20., 200, 0., 4.);
  hPrimaryDCAs = new TH1D("hPrimaryDCAs", "Primary track signed DCA; signed DCA (cm); Entries", 200, -4., 4.);
  hPrimaryDCAsVsPt = new TH2D("hPrimaryDCAsVsPt", "Primary track signed DCA vs. p_{T}; p_{T} (GeV/c); signed DCA (cm)", 200, 0., 20., 200, -4., 4.);
  hPrimaryDCAz = new TH1D("hPrimaryDCAz", "Primary track DCA_{z}; DCA_{z} (cm); Entries", 200, 0., 4.);
  hPrimaryDCAzVsPt = new TH2D("hPrimaryDCAzVsPt", "Primary track DCA_{z} vs. p_{T}; p_{T} (GeV/c); DCA_{z} (cm)", 200, 0., 20., 200, 0., 4.);
  hPrimaryDCAsVsDCAxy = new TH2D("hPrimaryDCAsVsDCAxy", "Primary track DCAxy vs. global DCAxy; global DCAxy (cm); DCAxy (cm)", 200, 0., 4., 200, 0., 4.);
  
    // PID QA histograms 
  hPrimaryTPCDedxVsP = new TH2D("hPrimaryTPCDedxVsP", "Primary track dE/dx vs momentum; p (GeV/c); dE/dx (keV/cm)", 200, 0., 20., 120, 0., 10.);
  hPrimaryTPCnSigmaPiVsP = new TH2D("hPrimaryTPCnSigmaPiVsP", "Primary track n#sigma_{#pi} vs momentum; p (GeV/c); n#sigma_{#pi}", 200, 0., 15., 200, -10., 10.);
  hPrimaryTPCnSigmaKVsP = new TH2D("hPrimaryTPCnSigmaKVsP", "Primary track n#sigma_{K} vs momentum; p (GeV/c); n#sigma_{K}", 200, 0., 15., 200, -10., 10.);
  hPrimaryTPCnSigmaPVsP = new TH2D("hPrimaryTPCnSigmaPVsP", "Primary track n#sigma_{p} vs momentum; p (GeV/c); n#sigma_{p}", 200, 0., 15., 200, -10., 10.);
  hPrimaryTPCnSigmaEVsP = new TH2D("hPrimaryTPCnSigmaEVsP", "Primary track n#sigma_{e} vs momentum; p (GeV/c); n#sigma_{e}", 200, 0., 15., 200, -10., 10.);

  // TOF QA histograms (
  hPrimaryTofInvBetaVsP = new TH2D("hPrimaryTofInvBetaVsP", "Primary track 1/#beta vs momentum; p (GeV/c); 1/#beta", 200, 0., 20., 200, 0.0, 4.0);
  hPrimaryTofMass2VsP = new TH2D("hPrimaryTofMass2VsP", "Primary track Mass^{2} vs momentum; p (GeV/c); m^{2} (GeV^{2}/c^{4})", 200, 0., 20., 200, 0., 2.);
  hPrimaryTofEtaVsPhi = new TH2D("hPrimaryTofEtaVsPhi", "Primary track #eta vs #phi (TOF-matched); #phi (rad); #eta", 360, -TMath::Pi(), TMath::Pi(), 300, -1.2, 1.2);
  hPrimaryTofMatchVsPt = new TH2D("hPrimaryTofMatchVsPt", "Primary track TOF matching flag vs p_{T}; p_{T} (GeV/c); TOF matching flag", 200, 0., 20., 2, -0.5, 1.5);

  // BEMC QA histograms
  hPrimaryBemcE = new TH1D("hPrimaryBemcE", "Primary track matched cluster energy; Energy (GeV); Entries", 500, 0., 50.);
  hPrimaryBemcEPVsPt = new TH2D("hPrimaryBemcEPVsPt", "Primary track E/p vs p_{T}; p_{T} (GeV/c); E/p", 200, 0., 20., 200, 0., 5.);
  hPrimaryBemcDeltaZVsPt = new TH2D("hPrimaryBemcDeltaZVsPt", "Primary track #Delta z vs p_{T}; p_{T} (GeV/c); #Delta z (cm)", 200, 0., 20., 200, -50., 50.);
  hPrimaryBemcDeltaPhiVsPt = new TH2D("hPrimaryBemcDeltaPhiVsPt", "Primary track #Delta #phi vs p_{T}; p_{T} (GeV/c); #Delta #phi (rad)", 200, 0., 20., 200, -0.1, 0.1);
  hPrimaryBemcDeltaZVsDeltaPhi = new TH2D("hPrimaryBemcDeltaZVsDeltaPhi", "Primary track #Delta z vs #Delta #phi; #Delta #phi (rad); #Delta z (cm)", 200, -0.1, 0.1, 200, -50., 50.);
  hPrimaryBsmdNEta = new TH1D("hPrimaryBsmdNEta", "Primary track BEMC matched cluster BSMD nEta; nEta; Entries", 20, -0.5, 19.5);
  hPrimaryBsmdNPhi = new TH1D("hPrimaryBsmdNPhi", "Primary track BEMC matched cluster BSMD nPhi; nPhi; Entries", 20, -0.5, 19.5);
  hPrimaryBtowDeltaEtaVsDeltaPhi = new TH2D("hPrimaryBtowDeltaEtaVsDeltaPhi", "Primary track #Delta #eta vs #Delta #phi (BTOW); #Delta #phi (rad); #Delta #eta", 200, -0.1, 0.1, 200, -0.1, 0.1);
  hPrimaryBemcEtaVsPhi = new TH2D("hPrimaryBemcEtaVsPhi", "Primary track #eta vs #phi (BEMC-matched); #phi (rad); #eta", 360, -TMath::Pi(), TMath::Pi(), 300, -1.2, 1.2);
  hPrimaryBtowE1VsId = new TH2D("hPrimaryBtowE1VsId", "Primary track matched cluster energy vs tower ID; Tower ID; Energy (GeV)", 4800, 0.5, 4800.5, 500, 0., 50.);

  // BBC QA histograms
  hBBCEastAdcVsId = new TH2D("hBBCEastAdcVsId", "East BBC ADC vs PMT ID; PMT ID; ADC", 25, 0.5, 24.5, 400, 0., 4000.);
  hBBCWestAdcVsId = new TH2D("hBBCWestAdcVsId", "West BBC ADC vs PMT ID; PMT ID; ADC", 25, 0.5, 24.5, 400, 0., 4000.);

  // Run Dependence histograms
  hBBCxVsRun = new TH2D("hBBCxVsRun", "BBCx vs Run; Run ID; BBCx", 3001, -1, 3000, 50, 0, 1e7);
  hVtxRankingVsRun = new TH2D("hVtxRankingVsRun", "Primary vertex ranking vs Run; Run ID; Primary vertex ranking", 3001, -1, 3000, 50, 0, 1e8);
  hNPrimariesVsRun = new TH2D("hNPrimariesVsRun", "# primary tracks/event vs Run; Run ID; # primary tracks/event", 3001, -1, 3000, 50, 0, 50);
  hNTofMatchedTracksVsRun = new TH2D("hNTofMatchedTracksVsRun", "# TOF-matched tracks/event vs Run; Run ID; # TOF-matched tracks/event", 3001, -1, 3000, 50, 0, 50);
  hDeltaVZVsRun = new TH2D("hDeltaVZVsRun", "#Delta Vz (TPC - VPD) vs Run; Run ID; #Delta Vz (cm)", 3001, -1, 3000, 50, -10., 10.);
  hVtxErrorXYVsRun = new TH2D("hVtxErrorXYVsRun", "Primary vertex error in xy vs Run; Run ID; Primary vertex error in xy (cm)", 3001, -1, 3000, 50, 0., 0.5);
  hVtxErrorZVsRun = new TH2D("hVtxErrorZVsRun", "Primary vertex error in z vs Run; Run ID; Primary vertex error in z (cm)", 3001, -1, 3000, 50, 0., 0.5);
  hNHitsFitVsRun = new TH2D("hNHitsFitVsRun", "nHitsFit vs Run; Run ID; nHitsFit", 3001, -1, 3000, 50, 0, 50);
  hNHitsDedxVsRun = new TH2D("hNHitsDedxVsRun", "nHitsDedx vs Run; Run ID; nHitsDedx", 3001, -1, 3000, 50, 0, 50);
  hNHitsFitRatioVsRun = new TH2D("hNHitsFitRatioVsRun", "nHitsFit/nHitsPoss vs Run; Run ID; nHitsFit/nHitsPoss", 3001, -1, 3000, 50, 0., 1.1);
  hDCAVsRun = new TH2D("hDCAVsRun", "DCA vs Run; Run ID; DCA (cm)", 3001, -1, 3000, 50, 0., 4.);
  hDedxVsRun = new TH2D("hDedxVsRun", "dE/dx vs Run; Run ID; dE/dx (keV/cm)", 3001, -1, 3000, 50, 0., 10.);
  hChi2VsRun = new TH2D("hChi2VsRun", "Primary track chi^{2}/ndf vs Run; Run ID; chi^{2}/ndf", 3001, -1, 3000, 50, 0., 10.);

  if (mDebug) {
    LOG_INFO << "All histograms have been created." << endm;
  }
}

//________________
void StPicoEASkimmer::CreateEATree()
{
  mEATree = new TTree("EATree", "Event and tracks information");

  // Event-level branches
  mEATree->Branch("eventID", &mEventID, "eventID/I");
  mEATree->Branch("runIndex", &mRunIndex, "runIndex/I");
  mEATree->Branch("vtxR", &mVtxR, "vtxR/F");
  mEATree->Branch("vtxZ", &mVtxZ, "vtxZ/F");
  mEATree->Branch("vtxVpdZ", &mVtxVpdZ, "vtxVpdZ/F");
  mEATree->Branch("vtxRanking", &mVtxRanking, "vtxRanking/F");
  mEATree->Branch("vtxErrorXY", &mVtxErrorXY, "vtxErrorXY/F");
  mEATree->Branch("vtxErrorZ", &mVtxErrorZ, "vtxErrorZ/F");
  mEATree->Branch("refMult", &mRefMult, "refMult/I");
  mEATree->Branch("gRefMult", &mGRefMult, "gRefMult/I");
  mEATree->Branch("nBTofMatch", &mNBTofMatch, "nBTofMatch/I");
  mEATree->Branch("nBEmcMatch", &mNBEmcMatch, "nBEmcMatch/I");
  mEATree->Branch("BBCx", &mBBCx, "BBCx/F");
  mEATree->Branch("ZDCx", &mZDCx, "ZDCx/F");
  mEATree->Branch("nPrimaries", &mNPrimaries, "nPrimaries/I");

  // BBC ADC signals (24 tiles each, using fixed-size arrays.
  mEATree->Branch("bbcAdcEast", mBbcAdcEast, "bbcAdcEast[24]/S");
  mEATree->Branch("bbcAdcWest", mBbcAdcWest, "bbcAdcWest[24]/S");

  // Event trigger IDs (vector of unsigned ints)
  mEATree->Branch("event_triggerIds", &mEventTriggerIds);

  // HT trigger details: flag, softId (tower id), adc
  mEATree->Branch("ht_flag", &mHtFlag);
  mEATree->Branch("ht_id", &mHtId);
  mEATree->Branch("ht_adc", &mHtAdc);

  // Track-level branches (std::vector for each variable)
  mEATree->Branch("track_pt", &mTrackPt);
  mEATree->Branch("track_eta", &mTrackEta);
  mEATree->Branch("track_phi", &mTrackPhi);
  mEATree->Branch("track_charge", &mTrackCharge);
  mEATree->Branch("track_nHitsFit", &mTrackNHitsFit);
  mEATree->Branch("track_nHitsDedx", &mTrackNHitsDedx);
  mEATree->Branch("track_nHitsRatio", &mTrackNHitsRatio);
  mEATree->Branch("track_chi2", &mTrackChi2);
  mEATree->Branch("track_dcaXY", &mTrackDCAxy);
  mEATree->Branch("track_dcaZ", &mTrackDCAz);
  mEATree->Branch("track_dcaS", &mTrackDCAs);
  mEATree->Branch("track_nSigmaPi", &mTrackNSigmaPi);
  mEATree->Branch("track_nSigmaK", &mTrackNSigmaK);
  mEATree->Branch("track_nSigmaP", &mTrackNSigmaP);
  mEATree->Branch("track_nSigmaE", &mTrackNSigmaE);
  mEATree->Branch("track_isTofTrack", &mTrackIsTofTrack);
  mEATree->Branch("track_btofBeta", &mTrackBTofBeta);
  mEATree->Branch("track_mass2", &mTrackMass2);
  mEATree->Branch("track_isBemcTrack", &mTrackIsBemcTrack);
  mEATree->Branch("track_bemcE", &mTrackBemcE);
  mEATree->Branch("track_bemcZDist", &mTrackBemcZDist);
  mEATree->Branch("track_bemcPhiDist", &mTrackBemcPhiDist);
  mEATree->Branch("track_btowId", &mTrackBtowId);
  mEATree->Branch("track_btowE", &mTrackBtowE);
  mEATree->Branch("track_btowPhiDist", &mTrackBtowPhiDist);
  mEATree->Branch("track_btowEtaDist", &mTrackBtowEtaDist);

  if (mDebug) {
    LOG_INFO << "TTree and branches have been created." << endm;
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
      } 
    } 
  }
  else {
    isGood = true;
  } 

  return isGood;
}

// Add a trigger id to the selection list (avoid duplicates)
void StPicoEASkimmer::addTriggerId(const unsigned int& id) {
  // Add if not present, then keep the vector sorted for binary_search lookups
  if (std::find(mTriggerId.begin(), mTriggerId.end(), id) == mTriggerId.end()) {
    mTriggerId.push_back(id);
    std::sort(mTriggerId.begin(), mTriggerId.end());
  }
}

//________________
Bool_t StPicoEASkimmer::EventCutForQA(StPicoEvent *event) {
  // Event-level QA cuts: vertex position and trigger
  const TVector3 &vtx = event->primaryVertex();
  return ( vtx.Z() >= mCutVtxZ[0] &&
           vtx.Z() <= mCutVtxZ[1] &&
           vtx.Perp() >= mCutVtxR[0] &&
           vtx.Perp() <= mCutVtxR[1] &&
           IsGoodTrigger(event) );
}

//________________
Bool_t StPicoEASkimmer::TrackCutForQA(StPicoTrack *track) {
  // Track-level QA cuts: number of hits, pt, eta, and hits ratio if available
  bool okHits = ( track->nHits() >= mCutNHits[0] && track->nHits() <= mCutNHits[1] );
  float pt = track->pPt();
  float eta = track->pMom().Eta();

  // nHits ratio: use nHitsFit / nHitsPoss if available (nHitsPoss method name may vary)
  // Fallback: if nHitsPoss()==0 then consider ratio test passed
  bool okHitsRatio = true;
  if (track->nHitsPoss() > 0) {
    float ratio = float(track->nHits()) / float(track->nHitsPoss());
    okHitsRatio = ( ratio >= mCutNHitsRatio[0] && ratio <= mCutNHitsRatio[1] );
  }

  return ( okHits && okHitsRatio && pt >= mCutPt[0] && pt <= mCutPt[1] && eta >= mCutEta[0] && eta <= mCutEta[1] );
}

//________________
Bool_t StPicoEASkimmer::EventCutForTree(StPicoEvent *event)
{
  // Tree-level event cuts: use a separate set of cuts for skimming
  const TVector3 &vtx = event->primaryVertex();
  float deltaVz = vtx.Z() - event->vzVpd();
  int nPrimaries = 0; // count primaries manually
  // Count primary tracks
  for (unsigned int i=0; i < mPicoDst->numberOfTracks(); ++i) {
    StPicoTrack *t = mPicoDst->track(i);
    if (t && t->isPrimary()) ++nPrimaries;
  }

  return ( vtx.Z() >= mTreeCutVtxZ[0] && vtx.Z() <= mTreeCutVtxZ[1] &&
           vtx.Perp() >= mTreeCutVtxR[0] && vtx.Perp() <= mTreeCutVtxR[1] &&
           deltaVz >= mTreeCutDeltaVz[0] && deltaVz <= mTreeCutDeltaVz[1] &&
           event->vzVpd() >= mTreeCutVtxVpdZ[0] && event->vzVpd() <= mTreeCutVtxVpdZ[1] &&
           nPrimaries >= mTreeCutNPrimariesMin &&
           IsGoodTrigger(event) );
}

//________________
Bool_t StPicoEASkimmer::TrackCutForTree(StPicoTrack *track)
{
  // Tree-level track cuts: stronger cuts for tracks stored in the TTree
  if (!track->isPrimary()) return false;

  if ( track->nHits() < mTreeCutNHits[0] || track->nHits() > mTreeCutNHits[1] ) return false;

  // nHitsRatio
  if (track->nHitsPoss() > 0) {
    float ratio = float(track->nHits()) / float(track->nHitsPoss());
    if ( ratio < mTreeCutNHitsRatio[0] || ratio > mTreeCutNHitsRatio[1] ) return false;
  }

  // nHitsDedx
  if ( track->nHitsDedx() < mTreeCutNHitsDedx[0] || track->nHitsDedx() > mTreeCutNHitsDedx[1] ) return false;

  // pT and eta
  float pt = track->pPt();
  if ( pt < mTreeCutPt[0] || pt > mTreeCutPt[1] ) return false;
  float eta = track->pMom().Eta();
  if ( eta < mTreeCutEta[0] || eta > mTreeCutEta[1] ) return false;

  // DCA: use gDCA with primary vertex if available, otherwise use gDCA(x,y,z) variant
  float dca = 0.;
  TVector3 vtx = mPicoDst->event()->primaryVertex();
  dca = track->gDCA(vtx.x(), vtx.y(), vtx.z());
  if ( dca < mTreeCutDCA[0] || dca > mTreeCutDCA[1] ) return false;

  return true;
}

void StPicoEASkimmer::LoadRunIndexMap(const char* filename) {

  std::ifstream fin(filename);
  if (!fin) {
    LOG_ERROR << "Cannot open run index file: " << filename << endm;
    return;
  }

  int runId;
  int runIndex = 0;
  while (fin >> runId) {
    mRunIndexMap[runId] = runIndex++;
  }

  std::cout << "Loaded " << mRunIndexMap.size() << " runs from " << filename << std::endl;
}

inline int StPicoEASkimmer::GetRunIndex(int runId) const {
  // Check if the map is empty or not initialized
  if (mRunIndexMap.empty()) {
    LOG_WARN << "Run index map is empty. Returning -1 for runId: " << runId << endm;
    return -1;
  }
  std::map<int, int>::const_iterator it = mRunIndexMap.find(runId);
  if (it != mRunIndexMap.end()) {
    return it->second;
  } else {
    LOG_WARN << "runId " << runId << " not found in run index map. Returning -1." << endm;
    return -1;
  }
}

  //________________
  Int_t StPicoEASkimmer::Make()
  {

    // Increment event counter
    mEventCounter++;
    hEventCounter->Fill(1);

    // Print event counter
    if ((mEventCounter % 10000) == 0)
    {
      // Avoid dereferencing mPicoDstMaker if this maker was constructed with a reader
      if (mPicoDstMaker && mPicoDstMaker->chain())
      {
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
  hEventCounter->Fill(2);

  // Check if event passes event cut (declared and defined in this
  // analysis maker)
  if ( !EventCutForQA(theEvent) ) {
    return kStOk;
  }
  hEventCounter->Fill(3);

  // Fill event QA histograms
  hVtxXVsY->Fill(theEvent->primaryVertex().X(), theEvent->primaryVertex().Y());
  hVtxZ->Fill(theEvent->primaryVertex().Z());
  hVtxVpdZ->Fill(theEvent->vzVpd());
  hDeltaVz->Fill(theEvent->primaryVertex().Z() - theEvent->vzVpd());
  hVtxZVsVpdZ->Fill(theEvent->vzVpd(), theEvent->primaryVertex().Z());
  hVtxRanking->Fill(theEvent->ranking());
  hVtxErrorXY->Fill(theEvent->primaryVertexError().Perp());
  hVtxErrorZ->Fill(theEvent->primaryVertexError().Z());
  //
  hRefMult->Fill(theEvent->refMult());
  hGRefMult->Fill(theEvent->grefMult());
  hRefMultVsGRefMult->Fill(theEvent->grefMult(), theEvent->refMult());
  hRefMultVsVz->Fill(theEvent->primaryVertex().Z(), theEvent->refMult());
  hNBTofMatch->Fill(theEvent->nBTOFMatch());
  hNBEmcMatch->Fill(theEvent->nBEMCMatch());
  //
  hBBCx->Fill(theEvent->BBCx());
  hZDCx->Fill(theEvent->ZDCx());
  hVtxErrorXYVsBBCx->Fill(theEvent->BBCx(), theEvent->primaryVertexError().Perp());
  hVtxErrorZVsBBCx->Fill(theEvent->BBCx(), theEvent->primaryVertexError().Z());
  hRefMultVsBBCx->Fill(theEvent->BBCx(), theEvent->refMult());
  hRefMultVsZDCx->Fill(theEvent->ZDCx(), theEvent->refMult());
  hNBTofMatchVsBBCx->Fill(theEvent->BBCx(), theEvent->nBTOFMatch());
  hNBTofMatchVsZDCx->Fill(theEvent->ZDCx(), theEvent->nBTOFMatch());

  // BBC QA histograms
  for (int iBBC=0; iBBC<24; iBBC++) {
    hBBCEastAdcVsId->Fill(iBBC, theEvent->bbcAdcEast(iBBC));
    hBBCWestAdcVsId->Fill(iBBC, theEvent->bbcAdcWest(iBBC));
  }

  // Run dependence QA histograms
  hBBCxVsRun->Fill(GetRunIndex(theEvent->runId()), theEvent->BBCx());
  hVtxRankingVsRun->Fill(GetRunIndex(theEvent->runId()), theEvent->ranking());
  hNTofMatchedTracksVsRun->Fill(GetRunIndex(theEvent->runId()), theEvent->nBTOFMatch());
  hDeltaVZVsRun->Fill(GetRunIndex(theEvent->runId()), theEvent->primaryVertex().Z() - theEvent->vzVpd());
  hVtxErrorXYVsRun->Fill(GetRunIndex(theEvent->runId()), theEvent->primaryVertexError().Perp());
  hVtxErrorZVsRun->Fill(GetRunIndex(theEvent->runId()), theEvent->primaryVertexError().Z());

  // Retrieve number of tracks in the event. Make sure that
  // SetStatus("Track*",1) is set to 1. In case of 0 the number
  // of stored tracks will be 0, even if those exist
  unsigned int nTracks = mPicoDst->numberOfTracks();
  int nPrimaries = 0; // Manually counting primaries
  if (nTracks == 0)
  {
    // No tracks in the event
    return kStOk;
  }
  hEventCounter->Fill(4);

  // Track loop
  for (unsigned int iTrk=0; iTrk<nTracks; iTrk++) {

    // Retrieve i-th pico track
    hTrackCounter->Fill(1);
    StPicoTrack *theTrack = (StPicoTrack*)mPicoDst->track(iTrk);
    // Track must exist
    if (!theTrack) continue;
    hTrackCounter->Fill(2);

    // Check if track passes track cut
    if ( !TrackCutForQA(theTrack) ) continue;
    hTrackCounter->Fill(3);

    // Primary track analysis
    if ( !theTrack->isPrimary() ) continue;
    hTrackCounter->Fill(4);
    nPrimaries++;

    // Fill primary track histograms
    hPrimaryPt->Fill(theTrack->pPt());
    hPrimaryEta->Fill(theTrack->pMom().Eta());
    hPrimaryPhi->Fill(theTrack->pMom().Phi());
    hPrimaryEtaVsPhi->Fill(theTrack->pMom().Phi(), theTrack->pMom().Eta());
    hPrimaryEtaVsPt->Fill(theTrack->pPt(), theTrack->pMom().Eta());
    hPrimaryPhiVsPt->Fill(theTrack->pPt(), theTrack->pMom().Phi());
    hPrimaryNHitsFit->Fill(theTrack->nHitsFit());
    hPrimaryNHitsFitVsPt->Fill(theTrack->pPt(), theTrack->nHitsFit());
    hPrimaryNHitsDedx->Fill(theTrack->nHitsDedx());
    hPrimaryNHitsDedxVsPt->Fill(theTrack->pPt(), theTrack->nHitsDedx());
    if (theTrack->nHitsPoss() > 0) {
      float nHitsFitRatio = static_cast<float>(theTrack->nHitsFit()) / theTrack->nHitsPoss();
      hPrimaryNHitsFitRatio->Fill(nHitsFitRatio);
      hPrimaryNHitsFitRatioVsPt->Fill(theTrack->pPt(), nHitsFitRatio);
    }
    hPrimaryChi2->Fill(theTrack->chi2());
    hPrimaryChi2VsPt->Fill(theTrack->pPt(), theTrack->chi2());
    // DCA to be stored in absolute values
    float dca = TMath::Abs(theTrack->gDCA(theEvent->primaryVertex().x(),
                                         theEvent->primaryVertex().y(),
                                         theEvent->primaryVertex().z()));
    float dcaxy = TMath::Abs(theTrack->gDCAxy(theEvent->primaryVertex().x(),
                                             theEvent->primaryVertex().y()));
    float dcaz = TMath::Abs(theTrack->gDCAz(theEvent->primaryVertex().z()));

    // Signed DCA: signed distance in xy between the pT vector and the DCA vector (different from DCAxy which is the xy of the DCA vector itself)
    float dcas = theTrack->gDCAs(theEvent->primaryVertex());

    hPrimaryDCA->Fill(dca);
    hPrimaryDCAVsPt->Fill(theTrack->pPt(), dca);
    hPrimaryDCAxy->Fill(dcaxy);
    hPrimaryDCAs->Fill(dcas);
    hPrimaryDCAsVsPt->Fill(theTrack->pPt(), dcas);
    hPrimaryDCAxyVsPt->Fill(theTrack->pPt(), dcaxy);
    hPrimaryDCAz->Fill(dcaz);
    hPrimaryDCAzVsPt->Fill(theTrack->pPt(), dcaz);
    hPrimaryDCAsVsDCAxy->Fill(dcaxy, TMath::Abs(theTrack->gDCAs(theEvent->primaryVertex())));
    // Fill TPC PID QA histograms
    hPrimaryTPCDedxVsP->Fill(theTrack->pPtot(), theTrack->dEdx());
    hPrimaryTPCnSigmaPiVsP->Fill(theTrack->pPtot(), theTrack->nSigmaPion());
    hPrimaryTPCnSigmaKVsP->Fill(theTrack->pPtot(), theTrack->nSigmaKaon());
    hPrimaryTPCnSigmaPVsP->Fill(theTrack->pPtot(), theTrack->nSigmaProton());
    hPrimaryTPCnSigmaEVsP->Fill(theTrack->pPtot(), theTrack->nSigmaElectron());
    // Run dependent tracking QA histograms
    hNHitsFitVsRun->Fill(GetRunIndex(theEvent->runId()), theTrack->nHitsFit());
    hNHitsDedxVsRun->Fill(GetRunIndex(theEvent->runId()), theTrack->nHitsDedx());
    if (theTrack->nHitsPoss() > 0) {
      float nHitsFitRatio = static_cast<float>(theTrack->nHitsFit()) / theTrack->nHitsPoss();
      hNHitsFitRatioVsRun->Fill(GetRunIndex(theEvent->runId()), nHitsFitRatio);
    }
    hDCAVsRun->Fill(GetRunIndex(theEvent->runId()), dca);
    hDedxVsRun->Fill(GetRunIndex(theEvent->runId()), theTrack->dEdx());
    hChi2VsRun->Fill(GetRunIndex(theEvent->runId()), theTrack->chi2());

    // Accessing TOF PID traits information.
    // TOF information is valid for primary tracks ONLY
    if ( theTrack->isTofTrack() ) {
      StPicoBTofPidTraits *TofPidTrait =
      (StPicoBTofPidTraits*)mPicoDst->btofPidTraits( theTrack->bTofPidTraitsIndex() );
      if (!TofPidTrait) continue;

      // Fill primary track TOF information
      hPrimaryTofInvBetaVsP->Fill(theTrack->pPtot(), TofPidTrait->btofBeta() > 0 ? 1.0/TofPidTrait->btofBeta() : 10.0);
      float mass2 = -9999.;
      if (TofPidTrait->btofBeta() > 0) {
        float beta = TofPidTrait->btofBeta();
        mass2 = theTrack->pPtot()*theTrack->pPtot()*(1.0/(beta*beta) - 1.0);
      }
      hPrimaryTofMass2VsP->Fill(theTrack->pPtot(), mass2);
      hPrimaryTofEtaVsPhi->Fill(theTrack->pMom().Phi(), theTrack->pMom().Eta());
    }
    
    hPrimaryTofMatchVsPt->Fill(theTrack->pPt(), theTrack->isTofTrack() ? 1 : 0);

    // Accessing BEMC PID traits information.
    if ( theTrack->isBemcTrack() ) {
      StPicoBEmcPidTraits *BemcPidTrait =
      (StPicoBEmcPidTraits*)mPicoDst->bemcPidTraits( theTrack->bemcPidTraitsIndex() );
      if (!BemcPidTrait) continue;

      // Fill primary track BEMC information
      hPrimaryBemcE->Fill(BemcPidTrait->bemcE());
      if (theTrack->pPtot() > 0.0) {
        hPrimaryBemcEPVsPt->Fill(theTrack->pPt(), BemcPidTrait->bemcE()/theTrack->pPtot());
      }
      hPrimaryBemcDeltaZVsPt->Fill(theTrack->pPt(), BemcPidTrait->bemcZDist());
      hPrimaryBemcDeltaPhiVsPt->Fill(theTrack->pPt(), BemcPidTrait->bemcPhiDist());
      hPrimaryBemcDeltaZVsDeltaPhi->Fill(BemcPidTrait->bemcPhiDist(), BemcPidTrait->bemcZDist());
      hPrimaryBsmdNEta->Fill(BemcPidTrait->bemcSmdNEta());
      hPrimaryBsmdNPhi->Fill(BemcPidTrait->bemcSmdNPhi());
      hPrimaryBtowDeltaEtaVsDeltaPhi->Fill(BemcPidTrait->btowPhiDist(), BemcPidTrait->btowEtaDist());
      hPrimaryBemcEtaVsPhi->Fill(theTrack->pMom().Phi(), theTrack->pMom().Eta());
      hPrimaryBtowE1VsId->Fill(BemcPidTrait->btowId(), BemcPidTrait->btowE());
    } 

    hTrackCounter->Fill(9);
  }

  // Fill QA histograms involving number of primary tracks
  hNPrimaries->Fill(nPrimaries);
  hNPrimariesVsBBCx->Fill(theEvent->BBCx(), nPrimaries);
  hNPrimariesVsZDCx->Fill(theEvent->ZDCx(), nPrimaries);
  hNPrimariesVsRun->Fill(GetRunIndex(theEvent->runId()), nPrimaries);

  hEventCounter->Fill(5);

  // Store skimmed event information in a tree
  if (!EventCutForTree(theEvent)) {
    return kStOk;
  }
  hEventCounter->Fill(6);

  mEventID = theEvent->eventId();
  mRunIndex = GetRunIndex(theEvent->runId());
  mVtxR = theEvent->primaryVertex().Perp();
  mVtxZ = theEvent->primaryVertex().Z();
  mVtxVpdZ = theEvent->vzVpd();
  mVtxRanking = theEvent->ranking();
  mVtxErrorXY = theEvent->primaryVertexError().Perp();
  mVtxErrorZ = theEvent->primaryVertexError().Z();
  mRefMult = theEvent->refMult();
  mGRefMult = theEvent->grefMult();
  mNBTofMatch = theEvent->nBTOFMatch();
  mNBEmcMatch = theEvent->nBEMCMatch();
  mBBCx = theEvent->BBCx();
  mZDCx = theEvent->ZDCx();
  mNPrimaries = nPrimaries;

  // BBC ADC signals
  for (int i = 0; i < 24; ++i)
  {
    mBbcAdcEast[i] = static_cast<Short_t>(theEvent->bbcAdcEast(i));
    mBbcAdcWest[i] = static_cast<Short_t>(theEvent->bbcAdcWest(i));
  }

  // Populate event trigger IDs for this event into the vector branch
  // Only keep those triggers that are in the allowed list mTriggerId.
  // If mTriggerId is empty, we store an empty vector.
  const std::vector<unsigned int> evtTriggers = theEvent->triggerIds();
  mEventTriggerIds.clear();
  if (!evtTriggers.empty() && !mTriggerId.empty()) {
    mEventTriggerIds.reserve(evtTriggers.size());
    for (size_t i = 0; i < evtTriggers.size(); ++i) {
      unsigned int id = evtTriggers[i];
      if (std::binary_search(mTriggerId.begin(), mTriggerId.end(), id)) {
        mEventTriggerIds.push_back(id);
      }
    }
  }

  // Clear HT trigger vectors before filling for this event
  mHtFlag.clear();
  mHtId.clear();
  mHtAdc.clear();

  // Loop over all EMC triggers in the event and select only HT triggers (HT0-HT3)
  const unsigned int nEmcTrigs = mPicoDst->numberOfEmcTriggers();
  const unsigned int htMask = 0xF; // mask for HT0-HT3 bits (lowest 4 bits)
  if (nEmcTrigs > 0) {
    // Reserve space for efficiency
    mHtFlag.reserve(nEmcTrigs);
    mHtId.reserve(nEmcTrigs);
    mHtAdc.reserve(nEmcTrigs);

    for (unsigned int i = 0; i < nEmcTrigs; ++i) {
      StPicoEmcTrigger *etrig = mPicoDst->emcTrigger(i);
      if (!etrig) continue; // skip if trigger object is missing

      unsigned int flag = etrig->flag();
      // Only keep triggers where HT bits are set (HT0-HT3)
      if ((flag & htMask) == 0) continue; // skip non-HT triggers

      // Store HT trigger details: flag (which HT), tower id (softId), and ADC value
      Short_t sflag = static_cast<Short_t>(flag & htMask); // which HT bit(s) are set
      Short_t sid = static_cast<Short_t>(etrig->id());     // tower softId
      Short_t sadc = static_cast<Short_t>(etrig->adc());   // ADC value

      mHtFlag.push_back(sflag);
      mHtId.push_back(sid);
      mHtAdc.push_back(sadc);
    }
  }

  // Clear track vectors
  mTrackPt.clear();
  mTrackEta.clear();
  mTrackPhi.clear();
  mTrackCharge.clear();
  mTrackNHitsFit.clear();
  mTrackNHitsDedx.clear();
  mTrackNHitsRatio.clear();
  mTrackChi2.clear();
  mTrackDCAxy.clear();
  mTrackDCAz.clear();
  mTrackDCAs.clear();
  mTrackNSigmaPi.clear();
  mTrackNSigmaK.clear();
  mTrackNSigmaP.clear();
  mTrackNSigmaE.clear();
  mTrackIsTofTrack.clear();
  mTrackBTofBeta.clear();
  mTrackMass2.clear();
  mTrackIsBemcTrack.clear();
  mTrackBemcE.clear();
  mTrackBemcZDist.clear();
  mTrackBemcPhiDist.clear();
  mTrackBtowId.clear();
  mTrackBtowE.clear();
  mTrackBtowPhiDist.clear();
  mTrackBtowEtaDist.clear();
  Int_t nTracksForTree = 0;

  // Loop over tracks again to fill track variables
  for (unsigned int iTrk=0; iTrk<nTracks; iTrk++) {
    StPicoTrack *theTrack = (StPicoTrack*)mPicoDst->track(iTrk);
    if (!theTrack) continue;

    if (!TrackCutForTree(theTrack)) continue;
    nTracksForTree++;
    
    // pT, eta, phi, charge
    mTrackPt.push_back(theTrack->pPt());
    mTrackEta.push_back(theTrack->pMom().Eta());
    mTrackPhi.push_back(theTrack->pMom().Phi());
    mTrackCharge.push_back(theTrack->charge());

    // nHitsFit, nHitsDedx, nHitsRatio
    mTrackNHitsFit.push_back(static_cast<Short_t>(theTrack->nHitsFit()));
    mTrackNHitsDedx.push_back(static_cast<Short_t>(theTrack->nHitsDedx()));
    if (theTrack->nHitsPoss() > 0) {
      mTrackNHitsRatio.push_back(static_cast<float>(theTrack->nHitsFit()) / theTrack->nHitsPoss());
    } else {
      mTrackNHitsRatio.push_back(0.0f);
    }

    // chi2
    mTrackChi2.push_back(theTrack->chi2());

    // DCA values (calculated as in previous track loop)
    float dcaXY = TMath::Abs(theTrack->gDCAxy(theEvent->primaryVertex().x(), theEvent->primaryVertex().y()));
    float dcaZ  = TMath::Abs(theTrack->gDCAz(theEvent->primaryVertex().z()));
    float dcaS  = theTrack->gDCAs(theEvent->primaryVertex());
    mTrackDCAxy.push_back(dcaXY);
    mTrackDCAz.push_back(dcaZ);
    mTrackDCAs.push_back(dcaS);

    // TPC PID
    mTrackNSigmaPi.push_back(theTrack->nSigmaPion());
    mTrackNSigmaK.push_back(theTrack->nSigmaKaon());
    mTrackNSigmaP.push_back(theTrack->nSigmaProton());
    mTrackNSigmaE.push_back(theTrack->nSigmaElectron());

    // TOF info (access via btofPidTraits)
    // track has to have Tof hit, Tof pid traits existing, and match flag > 0
    if (theTrack->isTofTrack()) {
      
      StPicoBTofPidTraits *TofPidTrait =
      (StPicoBTofPidTraits*)mPicoDst->btofPidTraits(theTrack->bTofPidTraitsIndex());

      if (TofPidTrait && TofPidTrait->btofMatchFlag() > 0) {
        mTrackIsTofTrack.push_back(1);
      } else {
        mTrackIsTofTrack.push_back(0);
      }
      if (TofPidTrait && TofPidTrait->btofBeta() > 0) {
        mTrackBTofBeta.push_back(TofPidTrait->btofBeta());
        float mass2 = theTrack->pPtot() * theTrack->pPtot() * (1.0 / (TofPidTrait->btofBeta() * TofPidTrait->btofBeta()) - 1.0);
        mTrackMass2.push_back(mass2);
      } else {
        mTrackBTofBeta.push_back(-9999.0f);
        mTrackMass2.push_back(-9999.0f);
      }
    } else {
      mTrackIsTofTrack.push_back(0);
      mTrackBTofBeta.push_back(-9999.0f);
      mTrackMass2.push_back(-9999.0f);
    }
    // BEMC info (access via bemcPidTraits)
    if (theTrack->isBemcTrack()) {
      mTrackIsBemcTrack.push_back(1);
      StPicoBEmcPidTraits *BemcPidTrait =
      (StPicoBEmcPidTraits*)mPicoDst->bemcPidTraits(theTrack->bemcPidTraitsIndex());
      if (BemcPidTrait) {
      mTrackBemcE.push_back(BemcPidTrait->bemcE());
      mTrackBemcZDist.push_back(BemcPidTrait->bemcZDist());
      mTrackBemcPhiDist.push_back(BemcPidTrait->bemcPhiDist());
      mTrackBtowId.push_back(static_cast<Short_t>(BemcPidTrait->btowId()));
      mTrackBtowE.push_back(BemcPidTrait->btowE());
      mTrackBtowPhiDist.push_back(BemcPidTrait->btowPhiDist());
      mTrackBtowEtaDist.push_back(BemcPidTrait->btowEtaDist());
      } else {
      mTrackBemcE.push_back(-9999.0f);
      mTrackBemcZDist.push_back(-9999.0f);
      mTrackBemcPhiDist.push_back(-9999.0f);
      mTrackBtowId.push_back(static_cast<Short_t>(-9999));
      mTrackBtowE.push_back(-9999.0f);
      mTrackBtowPhiDist.push_back(-9999.0f);
      mTrackBtowEtaDist.push_back(-9999.0f);
      }
    } else {
      mTrackIsBemcTrack.push_back(0);
      mTrackBemcE.push_back(-9999.0f);
      mTrackBemcZDist.push_back(-9999.0f);
      mTrackBemcPhiDist.push_back(-9999.0f);
      mTrackBtowId.push_back(static_cast<Short_t>(-9999));
      mTrackBtowE.push_back(-9999.0f);
      mTrackBtowPhiDist.push_back(-9999.0f);
      mTrackBtowEtaDist.push_back(-9999.0f);
    }
    }

    hEventCounter->Fill(7);
    if (nTracksForTree > 0) mEATree->Fill();

    hEventCounter->Fill(9);
    return kStOk;
  }
