# star-pico-skim — StPicoEASkimmer analysis and steering macro

This repository contains the analysis maker `StPicoEASkimmer` and its
steering macro `runPicoEASkimmer.C`. The code reads STAR picoDst files and
produces compact skim trees suitable for downstream analysis.

Key points
- StPicoEASkimmer (code in `StRoot/StPicoEASkimmer/`) is the analysis maker.
- The steering macro `runPicoEASkimmer.C` builds the maker, sets configuration
  and runs over input picoDst files or filelists.
- Cuts are defined in two levels inside the steering macro:
  - QA-level cuts: used for histograms and general QA (vertex ranges, nHits,
    pT, eta, etc.). These are useful to monitor data quality.
  - Tree-level (skimming) cuts: stricter selections used to decide which
    events and tracks are written into the compact TTree (the skimmed output).
- The output TTree contains only basic ROOT types (ints, floats, arrays and
  std::vectors) so the produced files can be analysed independently of
  `root4star`/`StRoot` (just use ROOT or RDataFrame, python uproot, etc.).

Repository structure (short)
- `runPicoEASkimmer.C`      — steering macro; set triggers, QA cuts, and tree cuts
- `StRoot/StPicoEASkimmer/` — analysis maker sources and headers
- `runlist2017.txt`         — runlist used by the class to define chronological run indices (for convenience)
- `templates/`              — XML template for scheduler submission (used by helper)
- `scripts/`                — helper scripts: `generate_xmls.py` and `submit_all.sh`
- `README_SUBMIT.md`       — submission-focused documentation (scheduler & XML)

Quick notes for users
- To change cuts or trigger selection, edit `runPicoEASkimmer.C` in the
  cut configuration block — QA and tree-level cuts are clearly separated.
- To produce a local test output, you can run the macro in ROOT interactively:

```bash
# example: run a single picoDst file and write local output
root -q -b -l runPicoEASkimmer.C\("/path/to/example.picoDst.root","test_out.root",1000\)

# or using a short filelist, running over all events
root -q -b -l runPicoEASkimmer.C\("/path/to/short_filelist.list","test_out.root",-1\)
```

- For batch submissions and provenance (XML generation, sandboxing, git
  snapshot), see `README_SUBMIT.md` which documents the scheduler workflow.

