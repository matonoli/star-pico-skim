# Submitting PicoEASkimmer jobs with STAR scheduler (star-submit)

This document describes a compact, easy-to-use workflow and the helper scripts
in this repository to generate STAR scheduler XML files, capture metadata, and
submit jobs via `star-submit`.

**Recommended usage:**  
Create and work inside an empty folder called `submit`.  
All script invocations below assume you are inside this `submit` directory, and the scripts are accessed via `../scripts/`.

Files used
- `templates/job_template.xml`  - XML template with placeholders filled by the generator
- `scripts/generate_xmls.py`    - Python3 script to create XML files and write metadata
- `scripts/submit_all.sh`       - tcsh helper to submit XML files (supports dry-run)


Quick example
1. Generate an XML into the GPFS project folder derived from the day tag.

  The generator creates two subdirectories under the GPFS base for the
  given daytag:
  - `<gpfs-base>/submission/<daytag>/` : XML, METADATA, steering snapshot, and scheduler lists
  - `<gpfs-base>/production/<daytag>/`  : final job outputs (`*.root`) and per-job stdout/stderr logs

```bash
../scripts/generate_xmls.py \
  --daytag 251012 \
  --input-catalog "trgsetupname=pp500_production_2017,production=P20ic,filename~st_physics,filetype=daq_reco_picoDst,storage=local" \
  --simulate  
```
or using a filelist:
```bash
../scripts/generate_xmls.py --daytag 251012 --input-list "../filelists/2017_pp_500GeV_picoDst_local.list" --simulate
```
2. Inspect generated XML and METADATA file in the `--output-dir`.

3. Submit (small test):

```tcsh
# perform a dry-run first to test if the script can locate the xml file
../scripts/submit_all.sh /gpfs/mnt/gpfs01/star/pwg/matonoli/ea-trees-2017-pp500/submission/251012/job_251012.xml --dry-run
# then run without --dry-run to actually submit to the STAR scheduler
../scripts/submit_all.sh /gpfs/mnt/gpfs01/star/pwg/matonoli/ea-trees-2017-pp500/submission/251012/job_251012.xml
```

Best practices included
- The generator derives the GPFS output folder automatically from `--daytag` so
  you only need to provide the day string (reduces repeated typing and errors).
- Use `simulateSubmission="true"` for a full scheduler test run before real
  submissions.
- Capture a `METADATA_YYMMDD.txt` containing generation time and git commit
  hash and a snapshot of `runPicoEASkim.C` for provenance.
- Job resource settings set by the template: maxFilesPerProcess=50,
  softLimits="true", filesPerHour=40. These can be tuned in the template if
  needed.

Suggested small-test plan
1. Dry-run: generate XML with `--simulate` and run `submit_all.sh --dry-run`. Inspect the `logs/` and `METADATA_` file.
2. Single-job test: if you can restrict the catalog to a small set via `nFiles` in the generated `<input>` element, do so; alternatively create a short `test.list` with a few xrootd file URLs and pass it with `--input-list` and submit a single xml.
3. Verify: after the job completes, check outputs in `<gpfs-base>/production/251012/` and scheduler artifacts (xml, metadata, csh, lists) in `<gpfs-base>/submission/251012/`.

Requirements
- This repository already contains `runPicoEASkim.C` used as the steering macro. The generator snapshots it.
- `star-submit` and the STAR environment (starver, runtimes) are available on the submission host.
- The GPFS destination must be writeable by the jobs. Adjust `--gpfs-dest` accordingly.

