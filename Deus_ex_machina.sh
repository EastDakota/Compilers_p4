#!/bin/bash
# Insert local project repository here V
P4_DIR='/cs/home/stu/scott6da/cs432/p4-codegen/'
INPUT_FILES=$P4_DIR'tests/inputs/'
EXP_OUTPUT_REPO=$P4_DIR'tests/helper_expected_outputs/'
ACT_OUTPUT_REPO=$P4_DIR'tests/helper_actual_outputs/'
ACTUAL_DECAF_CALL=$P4_DIR'decaf'
CORRECT_DECAF_CALL='/cs/students/cs432/f22/decaf'
make
# Checks to see if output directories are initialized
if [ ! -d "$EXP_OUTPUT_REPO" ];
then 
    mkdir $EXP_OUTPUT_REPO
fi
if [ ! -d "$ACT_OUTPUT_REPO" ];
then 
    mkdir $ACT_OUTPUT_REPO
fi
# Clears the current info in REPOS
rm -f $ACT_OUTPUT_REPO* $EXP_OUTPUT_REPO*
for filename in $INPUT_FILES*; do
    # Changes file permissions for basename
    chmod 744 $filename 
    # Changing the file names to make output easier
    DECAF_FILE=$(basename ${filename})
    FILE_NAME_NO_EXTENSION=${DECAF_FILE::-6}
    # Running the actual and expected, make a diff file
    
    touch $ACT_OUTPUT_REPO$FILE_NAME_NO_EXTENSION.txt
    touch $EXP_OUTPUT_REPO$FILE_NAME_NO_EXTENSION.txt
    $ACTUAL_DECAF_CALL  $filename > $ACT_OUTPUT_REPO$FILE_NAME_NO_EXTENSION.txt
    $CORRECT_DECAF_CALL --fdump-iloc $filename > $EXP_OUTPUT_REPO$FILE_NAME_NO_EXTENSION.txt
    # Diff File
    diff -u $ACT_OUTPUT_REPO$FILE_NAME_NO_EXTENSION.txt $EXP_OUTPUT_REPO$FILE_NAME_NO_EXTENSION.txt > $ACT_OUTPUT_REPO$FILE_NAME_NO_EXTENSION.diff
    echo Running: $DECAF_FILE 
done
# Credit to Dr. Lam, although this program does not create tests
echo "========================================"
echo " NOTICE: THIS TESTSUITE IS INCOMPLETE."
echo "YOU WILL BE GRADED ON OTHER TEST CASES."
echo "        WRITE YOUR OWN TESTS!"
exit 0