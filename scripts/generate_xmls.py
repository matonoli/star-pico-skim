#!/usr/bin/env python3
"""
generate_xmls.py

Create STAR scheduler XML files from the template in `templates/job_template.xml`.
Features:
 - Fill placeholders: __BASEDIR__, __SRCDIR__, __GPFS_DEST__, __DAYTAG__, __OUTPREFIX__
 - Accept either a catalog query string or a local filelist path
 - Toggle simulateSubmission (true/false)
 - Record git commit hash and copy a snapshot of `runPicoEASkim.C` into the output dir

Usage examples in README_SUBMIT.md

"""
import argparse
import os
import shutil
import subprocess
import datetime
import sys


TEMPLATE = os.path.join(os.path.dirname(__file__), '..', 'templates', 'job_template.xml')


def read_template():
    with open(TEMPLATE, 'r') as f:
        return f.read()


def git_commit_hash(repo_dir):
    try:
        out = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=repo_dir, stderr=subprocess.DEVNULL)
        return out.decode().strip()
    except Exception:
        return 'UNKNOWN'


def write_snapshot(src_path, dest_dir):
    os.makedirs(dest_dir, exist_ok=True)
    basename = os.path.basename(src_path)
    dest = os.path.join(dest_dir, basename)
    try:
        shutil.copy2(src_path, dest)
        return dest
    except Exception:
        return None


def build_input_element(args):
    # If catalog provided, build catalog input. Else use filelist path.
    if args.input_catalog:
        # wrap catalog in catalog:... and request nFiles=all
        return f'<input URL="catalog:star.bnl.gov?{args.input_catalog}" nFiles="all" />'
    elif args.input_list:
        # ensure absolute path
        path = os.path.abspath(args.input_list)
        return f'<input URL="filelist:{path}" nFiles="all" />'
    else:
        raise RuntimeError('Either --input-catalog or --input-list must be provided')


def main():
    parser = argparse.ArgumentParser(description='Generate STAR scheduler XML files for PicoEASkimmer')
    parser.add_argument('--output-dir', required=False, help='Optional explicit output dir; if omitted it will be derived from gpfs-base and daytag')
    parser.add_argument('--basedir', default=None, help='Base working dir for logs/lists inside xml (defaults to gpfs dest)')
    parser.add_argument('--srcdir', default=os.path.abspath(os.path.join(os.path.dirname(__file__), '..')),
                        help='Path to repo to include in sandbox')
    parser.add_argument('--gpfs-base', default='/gpfs/mnt/gpfs01/star/pwg/matonoli/ea-trees-2017-pp500',
                        help='Base GPFS directory under which day-tagged subfolders will be created (default project path)')
    parser.add_argument('--daytag', required=True, help='Day tag YYMMDD to include in output names')
    parser.add_argument('--out-prefix', default='eaTree', help='Prefix for output root files')
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('--input-catalog', help='Catalog query string (without catalog:star.bnl.gov?)')
    group.add_argument('--input-list', help='Path to a local filelist (xrootd syntax)')
    parser.add_argument('--simulate', action='store_true', help='Set simulateSubmission="true" for dry-run')
    parser.add_argument('--xml-name', default=None, help='Optional explicit xml filename')
    args = parser.parse_args()

    # derive gpfs destination and output dir from daytag if output-dir not provided
    if args.output_dir:
        submission_dir = os.path.abspath(args.output_dir)
    else:
        # default submission dir: gpfs_base/submission/<daytag>
        submission_dir = os.path.join(args.gpfs_base, 'submission', args.daytag)
    # production/output directory: gpfs_base/production/<daytag>
    output_dir = os.path.join(args.gpfs_base, 'production', args.daytag)
    gpfs_dest = output_dir
    os.makedirs(submission_dir, exist_ok=True)
    os.makedirs(output_dir, exist_ok=True)
    # read template
    tpl = read_template()

    input_element = build_input_element(args)

    simulate_val = 'true' if args.simulate else 'false'

    # try to detect git commit
    commit = git_commit_hash(args.srcdir)

    # copy steering macro
    steering_src = os.path.join(args.srcdir, 'runPicoEASkim.C')
    steering_snapshot = write_snapshot(steering_src, os.path.join(args.output_dir, 'steering_snapshot'))

    # fill placeholders
    day = args.daytag
    # baseDir defaults to the submission area unless user passes a basedir
    basedir = args.basedir if args.basedir else submission_dir

    # Build sandbox files list: include StRoot/, steering macro, and compiled runtime dir
    # If .sl73_gcc485 exists in srcdir, include it.
    sandbox_files = []
    sandbox_files.append(f'      <File>file:{args.srcdir}/StRoot/</File>')
    sandbox_files.append(f'      <File>file:{args.srcdir}/runPicoEASkim.C</File>')
    sl_dir = os.path.join(args.srcdir, '.sl73_gcc485')
    if os.path.isdir(sl_dir):
        sandbox_files.append(f'      <File>file:{sl_dir}</File>')

    sandbox_block = '\n'.join(sandbox_files)

    base_subs = {
        '__BASEDIR__': basedir,
        '__SRCDIR__': args.srcdir,
        '__SUBMISSION_DIR__': submission_dir,
        '__OUTPUT_DIR__': output_dir,
        '__GPFS_DEST__': gpfs_dest,
        '__DAYTAG__': day,
        '__OUTPREFIX__': args.out_prefix,
        '__INPUT__': input_element,
        '__SIMULATE__': simulate_val,
        '__SANDBOX_FILES__': sandbox_block,
    }

    xml = tpl
    for k, v in base_subs.items():
        xml = xml.replace(k, v)

    # write xml file
    xml_name = args.xml_name or f'pico_ea_{day}.xml'
    xml_path = os.path.join(submission_dir, xml_name)
    with open(xml_path, 'w') as f:
        f.write(xml)

    # write metadata file
    meta_path = os.path.join(submission_dir, f'METADATA_{day}.txt')
    with open(meta_path, 'w') as f:
        f.write(f'generated: {datetime.datetime.utcnow().isoformat()}Z\n')
        f.write(f'git_commit: {commit}\n')
        f.write(f'steering_snapshot: {steering_snapshot or "<not-copied>"}\n')
        f.write(f'xml: {xml_path}\n')
        f.write(f'input_element: {input_element}\n')

    print('Wrote:', xml_path)
    print('Metadata:', meta_path)
    if steering_snapshot:
        print('Steering copied to:', steering_snapshot)
    print('Done')


if __name__ == '__main__':
    main()
