#ifndef StPicoEASkimmer_h
#define StPicoEASkimmer_h

// STAR headers
#include "StMaker.h"

// C++ headers
#include <vector>
#include <iostream>

//
// Forward declarations
//

// StPicoDstMaker
class StPicoDstMaker;

// StPicoEvent
class StPicoDst;
class StPicoDstReader;
class StPicoEvent;
class StPicoTrack;

// ROOT
class TFile;
class TH1F;
class TH1D;
class TH2F;
class TH2D;

//________________
class StPicoEASkimmer : public StMaker {

 public:
  /// Constructor
  /// \param maker Takes a pointer to StPicoDstMaker that was initialized
  ///              in the launching macro and passed as an argument
  /// \param oFileName Name of the output file where the histograms will
  ///                  be stored
  /// This constructor provides an example of how to use StPicoDstMaker/StPicoDstMaker.h(cxx)
  StPicoEASkimmer(StPicoDstMaker *maker,
                         const char* oFileName = "oStPicoEASkimmer.root" );
  /// Constructor
  /// \param inFileName Name of the input name.picoDst.root file or a name.lis(t)
  ///                   file with a list of name.picoDst.root files
  /// \param oFileName Name of the output file where the histograms will
  ///                   be stored
  /// This constructor shows how to access data using StPicoEvent/StPicoDstReader.h(cxx)
  StPicoEASkimmer(const char* inFileName,
                         const char* oFileName = "oStPicoEASkimmer.root");
  /// Destructor
  virtual ~StPicoEASkimmer();

  /// Init method inherited from StMaker
  virtual Int_t Init();
  /// Make method inherited from StMaker
  virtual Int_t Make();
  /// Finish method inherited from StMaker
  virtual Int_t Finish();

  /// Load run index map from a file
  void LoadRunIndexMap(const char* filename);
  /// Get run index for a given run ID
  int GetRunIndex(int runId) const;

  //
  // Setters
  //

  /// Set debug status
  void setDebugStatus(bool status)                      { mDebug = status; }
  /// Set output file name
  void setOutputFileName(const char* name)              { mOutFileName = name; }

  /// Add trigger id to select. Avoids adding duplicates.
  /// Triggers are the numeric IDs stored in StPicoEvent trigger list.
  void addTriggerId(const unsigned int& id);
  /// Set cut on z-position of the primary vertex
  void setVtxZ(const float& lo, const float& hi)        { mCutVtxZ[0]=lo; mCutVtxZ[1]=hi; }
  /// Set cut on the radial position of the primary vertex
  void setVtxR(const float& lo, const float& hi)        { mCutVtxR[0]=lo; mCutVtxR[1]=hi; }

  /// Set cut on nHitsFit/nHitsPoss ratio for QA
  void setNHitsRatio(const float& lo, const float& hi) { mCutNHitsRatio[0] = lo; mCutNHitsRatio[1] = hi; }

  /// Set cut on nHits
  void setNHits(const int& lo, const int& hi)           { mCutNHits[0] = lo; mCutNHits[1] = hi; }
  /// Set cut on track pT
  void setPt(const float& lo, const float& hi)          { mCutPt[0] = lo; mCutPt[1] = hi; }
  /// Set cut on track pseudorapidity
  void setEta(const float& lo, const float& hi)         { mCutEta[0] = lo; mCutEta[1] = hi; }

  // --------------------------
  // Cuts used specifically for skimming to the TTree (tree-level cuts)
  // These are separate from the QA cuts above and can be set independently
  // Events: vtxZ, vtxR, deltaVz (TPC - VPD), vtxVpdZ (VPD z), min number of primaries
  void setTreeVtxZ(const float& lo, const float& hi)    { mTreeCutVtxZ[0]=lo; mTreeCutVtxZ[1]=hi; }
  void setTreeVtxR(const float& lo, const float& hi)    { mTreeCutVtxR[0]=lo; mTreeCutVtxR[1]=hi; }
  void setTreeDeltaVz(const float& lo, const float& hi) { mTreeCutDeltaVz[0]=lo; mTreeCutDeltaVz[1]=hi; }
  void setTreeVtxVpdZ(const float& lo, const float& hi) { mTreeCutVtxVpdZ[0]=lo; mTreeCutVtxVpdZ[1]=hi; }
  void setTreeNPrimariesMin(const int& min)             { mTreeCutNPrimariesMin = min; }

  // Track-level tree cuts
  void setTreeNHits(const int& lo, const int& hi)       { mTreeCutNHits[0]=lo; mTreeCutNHits[1]=hi; }
  void setTreeNHitsRatio(const float& lo, const float& hi) { mTreeCutNHitsRatio[0]=lo; mTreeCutNHitsRatio[1]=hi; }
  void setTreeNHitsDedx(const int& lo, const int& hi)   { mTreeCutNHitsDedx[0]=lo; mTreeCutNHitsDedx[1]=hi; }
  void setTreePt(const float& lo, const float& hi)      { mTreeCutPt[0]=lo; mTreeCutPt[1]=hi; }
  void setTreeEta(const float& lo, const float& hi)     { mTreeCutEta[0]=lo; mTreeCutEta[1]=hi; }
  void setTreeDCA(const float& lo, const float& hi)     { mTreeCutDCA[0]=lo; mTreeCutDCA[1]=hi; }


 private:

  /// Create histograms
  void CreateHistograms();

  /// Create the skim tree and define branches
  void CreateEATree();

  /// Check the at least one triggers to select is in the event triggers list
  Bool_t IsGoodTrigger(StPicoEvent *event);

  /// Event cut for which events to analyse
  Bool_t EventCutForQA(StPicoEvent *event);

  /// Track cut for which tracks to analyse
  Bool_t TrackCutForQA(StPicoTrack *track);

  /// Event cut for skimming to smaller trees
  Bool_t EventCutForTree(StPicoEvent *event);

  /// Track cut for skimming to smaller trees
  Bool_t TrackCutForTree(StPicoTrack *track);

  /// Debug mode
  Bool_t mDebug;

  /// List of triggers to select
  std::vector<unsigned int> mTriggerId;
  /// z-position of the primary vertex [min,max]
  Float_t mCutVtxZ[2];
  /// Radial position of the primary vertex [min,max]
  Float_t mCutVtxR[2];

  /// nHitsFit / nHitsPoss ratio [min,max] for QA
  Float_t mCutNHitsRatio[2];

  /// nHits [min,max]
  Short_t mCutNHits[2];
  /// Transverse momentum [min,max]
  Float_t mCutPt[2];
  /// PseudoRapidity [min,max]
  Float_t mCutEta[2];

  // ----------------------
  // Tree-level (skimming) cuts
  Float_t mTreeCutVtxZ[2];
  Float_t mTreeCutVtxR[2];
  Float_t mTreeCutDeltaVz[2];
  Float_t mTreeCutVtxVpdZ[2];
  int     mTreeCutNPrimariesMin;

  // Track-level tree cuts
  Short_t mTreeCutNHits[2];
  Float_t mTreeCutNHitsRatio[2];
  Short_t mTreeCutNHitsDedx[2];
  Float_t mTreeCutPt[2];
  Float_t mTreeCutEta[2];
  Float_t mTreeCutDCA[2];

  /// Output file name
  const char* mOutFileName;
  /// Output file
  TFile *mOutFile;

  // Run index map
  std::map<int, int> mRunIndexMap;
  
  /// Pointer to StPicoDstMaker
  StPicoDstMaker *mPicoDstMaker;
  /// Instead of StPicoDstMaker one can use StPicoDstReader
  StPicoDstReader *mPicoDstReader;
  
  // Pointer to StPicoDst
  StPicoDst *mPicoDst;

  // Event counter
  TH1F *hEventCounter;
  TH1F *hTrackCounter;

  // Event-level QA histograms
  TH2F *hVtxXVsY;
  TH1F *hVtxZ;
  TH1F *hVtxVpdZ;
  TH1F *hDeltaVz;
  TH2F *hVtxZVsVpdZ;
  TH1F *hVtxRanking;
  TH1F *hVtxErrorXY;
  TH1F *hVtxErrorZ;
  //
  TH1F *hRefMult;
  TH1F *hGRefMult;
  TH2F *hRefMultVsGRefMult;
  TH2F *hRefMultVsVz;
  TH1F *hNPrimaries;
  TH1F *hNBTofMatch;
  TH1F *hNBEmcMatch;
  //
  TH1F *hBBCx;
  TH1F *hZDCx;
  TH2F *hVtxErrorXYVsBBCx;
  TH2F *hVtxErrorZVsBBCx;
  TH2F *hRefMultVsBBCx;
  TH2F *hNPrimariesVsBBCx;
  TH2F *hNBTofMatchVsBBCx;
  TH2F *hRefMultVsZDCx;
  TH2F *hNPrimariesVsZDCx;
  TH2F *hNBTofMatchVsZDCx;

  // Track-Level QA histograms
  TH1D *hPrimaryPt;
  TH1D *hPrimaryEta;
  TH1D *hPrimaryPhi;
  TH2D *hPrimaryEtaVsPhi;
  TH2D *hPrimaryEtaVsPt;
  TH2D *hPrimaryPhiVsPt;
  TH1D *hPrimaryNHitsFit;
  TH2D *hPrimaryNHitsFitVsPt;
  TH1D *hPrimaryNHitsDedx;
  TH2D *hPrimaryNHitsDedxVsPt;
  TH1D *hPrimaryNHitsFitRatio;
  TH2D *hPrimaryNHitsFitRatioVsPt;
  TH1D *hPrimaryChi2;
  TH2D *hPrimaryChi2VsPt;
  TH1D *hPrimaryDCA;
  TH2D *hPrimaryDCAVsPt;
  TH1D *hPrimaryDCAs;
  TH2D *hPrimaryDCAsVsPt;
  TH1D *hPrimaryDCAxy;
  TH2D *hPrimaryDCAxyVsPt;
  TH1D *hPrimaryDCAz;
  TH2D *hPrimaryDCAzVsPt;
  TH2D *hPrimaryDCAsVsDCAxy;

  // PID QA histograms
  TH2D *hPrimaryTPCDedxVsP;
  TH2D *hPrimaryTPCnSigmaPiVsP;
  TH2D *hPrimaryTPCnSigmaKVsP;
  TH2D *hPrimaryTPCnSigmaPVsP;
  TH2D *hPrimaryTPCnSigmaEVsP;

  // TOF QA histograms
  TH2D *hPrimaryTofInvBetaVsP;
  TH2D *hPrimaryTofMass2VsP;
  TH2D *hPrimaryTofEtaVsPhi;
  TH2D *hPrimaryTofMatchVsPt;

  // BEMC QA histograms
  TH1D *hPrimaryBemcE;
  TH2D *hPrimaryBemcEPVsPt;
  TH2D *hPrimaryBemcDeltaZVsPt;
  TH2D *hPrimaryBemcDeltaPhiVsPt;
  TH2D *hPrimaryBemcDeltaZVsDeltaPhi;
  TH1D *hPrimaryBsmdNEta;
  TH1D *hPrimaryBsmdNPhi;
  TH2D *hPrimaryBtowDeltaEtaVsDeltaPhi;
  TH2D *hPrimaryBemcEtaVsPhi;
  TH2D *hPrimaryBtowE1VsId;

  // BBC QA histograms
  TH2D *hBBCEastAdcVsId;
  TH2D *hBBCWestAdcVsId;

  // Run Dependence histograms
  TH2D *hBBCxVsRun;
  TH2D *hVtxRankingVsRun;
  TH2D *hNPrimariesVsRun;
  TH2D *hNTofMatchedTracksVsRun;
  TH2D *hDeltaVZVsRun;
  TH2D *hVtxErrorXYVsRun;
  TH2D *hVtxErrorZVsRun;
  TH2D *hNHitsFitVsRun;
  TH2D *hNHitsDedxVsRun;
  TH2D *hNHitsFitRatioVsRun;
  TH2D *hDCAVsRun;
  TH2D *hDedxVsRun;
  TH2D *hChi2VsRun;

  // Skim tree
  TTree *mEATree;

  // Event-level variables for tree
  Int_t mEventID;
  Int_t mRunIndex;
  Float_t mVtxR;
  Float_t mVtxZ;
  Float_t mVtxVpdZ;
  Float_t mVtxRanking;
  Float_t mVtxErrorXY;
  Float_t mVtxErrorZ;
  Int_t mRefMult;
  Int_t mGRefMult;
  Int_t mNBTofMatch;
  Int_t mNBEmcMatch;
  Float_t mBBCx;
  Float_t mZDCx;
  Int_t mNPrimaries;

  // Trigger IDs that fired for the current event (stored in the tree)
  std::vector<unsigned int> mEventTriggerIds;

  // High-tower (HT) trigger info: store only triggers with HT bits (ht0, ht1, ht3)
  // For each matching StPicoEmcTrigger we store: flag, softId (tower id), and adc
  std::vector<Short_t> mHtFlag;
  std::vector<Short_t> mHtId;
  std::vector<Short_t> mHtAdc;

  // BBC ADC signals (fixed-size arrays for 24 tiles)
  Short_t mBbcAdcEast[24];
  Short_t mBbcAdcWest[24];

  // Track-level variables (vectors)
  std::vector<Float_t> mTrackPt;
  std::vector<Float_t> mTrackEta;
  std::vector<Float_t> mTrackPhi;
  std::vector<Short_t> mTrackCharge;
  std::vector<Short_t> mTrackNHitsFit;
  std::vector<Short_t> mTrackNHitsDedx;
  std::vector<Float_t> mTrackNHitsRatio;
  std::vector<Float_t> mTrackChi2;
  std::vector<Float_t> mTrackDCAxy;
  std::vector<Float_t> mTrackDCAz;
  std::vector<Float_t> mTrackDCAs;
  std::vector<Float_t> mTrackNSigmaPi;
  std::vector<Float_t> mTrackNSigmaK;
  std::vector<Float_t> mTrackNSigmaP;
  std::vector<Float_t> mTrackNSigmaE;
  std::vector<Char_t> mTrackIsTofTrack;
  std::vector<Float_t> mTrackBTofBeta;
  std::vector<Float_t> mTrackMass2;
  std::vector<Char_t> mTrackIsBemcTrack;
  std::vector<Float_t> mTrackBemcE;
  std::vector<Float_t> mTrackBemcZDist;
  std::vector<Float_t> mTrackBemcPhiDist;
  std::vector<Short_t> mTrackBtowId;
  std::vector<Float_t> mTrackBtowE;
  std::vector<Float_t> mTrackBtowPhiDist;
  std::vector<Float_t> mTrackBtowEtaDist;

  /// Event counter
  UInt_t mEventCounter;

  // Constructor switcher in order to distinguish between StPicoDstMaker and StPicoDstReader
  //(needed for the example only). In real life, one should use only one of them.
  Bool_t mIsFromMaker;

  ClassDef(StPicoEASkimmer, 0)
};

#endif // StPicoEASkimmer_h
