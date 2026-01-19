#!/bin/tcsh
# submit_all.sh
# Iterate over .xml files in a directory and submit them with star-submit
# Usage: ./submit_all.sh /path/to/xmls [--dry-run]

# Check if at least one argument is provided
if ($#argv < 1) then
    echo "Usage: $0 <xml-file|xml-dir> [--all] [--dry-run]"
    echo "  If a directory is passed, use --all to submit every .xml inside it."
    exit 1
endif

# Set the first argument as the target (file or directory)
set target = $argv[1]
set all = 0      # Flag for submitting all XMLs in a directory
set dry = 0      # Flag for dry-run mode (no actual submission)
@ i = 2
# Parse additional arguments (--all and --dry-run)
while ($i <= $#argv)
    if ($argv[$i] == "--all") then
        set all = 1
    else if ($argv[$i] == "--dry-run") then
        set dry = 1
    endif
    @ i++
end

# If target is a directory, require --all to submit all XMLs inside
if (-d $target) then
    if (! $all) then
        echo "ERROR: target is a directory. Add --all to submit all xmls inside."
        exit 2
    endif
    # Get list of all .xml files in the directory
    set xml_list = ( $target/*.xml )
# If target is a file, just use that file
else if (-f $target) then
    set xml_list = ( $target )
# If target does not exist, exit with error
else
    echo "ERROR: target $target not found"
    exit 3
endif

# Set up log directory and log file for this submission run
set logdir = `dirname $xml_list[1]`/submit_logs
mkdir -p $logdir
set timestamp = `date +%Y%m%d-%H%M%S`
set record = $logdir/submissions_$timestamp.txt
echo "Submission run: $timestamp" > $record

# Loop over each XML file and submit (or dry-run)
foreach xml ($xml_list)
    echo "Processing $xml" | tee -a $record
    if ($dry) then
        # If dry-run, just print the command
        echo "DRY RUN: star-submit $xml" | tee -a $record
    else
        # Actually submit the XML file
        echo "Submitting: star-submit $xml" | tee -a $record
        # Run star-submit and show stdout+stderr immediately while also
        # appending to the record file. Capture star-submit's exit code by
        # writing it from the subshell into a temporary file.
        set tmp = "$logdir/rc_$$.tmp"
        ( star-submit $xml ; echo $status > $tmp ) |& tee -a $record
        if ( -f $tmp ) then
            set rc = `cat $tmp`
            rm -f $tmp
        else
            # if tmp wasn't written something unexpected happened
            set rc = 1
        endif
        # If submission failed, log the error code
        if ($rc != 0) then
            echo "star-submit returned $rc for $xml" | tee -a $record
        endif
    endif
end

# Print completion message and location of the log file
echo "Done. Record: $record"
