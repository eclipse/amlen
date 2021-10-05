REM Batch file to run Term Extract on a set of source files

REM to run locally uncomment the first entry for TERMEXT_DIR below, and
REM set the path for TERMEXT_DIR to the directory where you have Term
REM Extract installed locally 

REM set TERMEXT_DIR=C:\TermExt
set TERMEXT_DIR=%TOOLS_DIR%\w2008_64\TermExt

REM create list of files to run TermExt on
dir *.dita /s /b > filelist.txt
dir *.ditamap /s /b >> filelist.txt

REM set a variable corresponding to directory this batch file
REM is being run from - this equates to the docSource value
REM in the Ant build.xml file
set dir=%~dp0

REM change directory to the Term Extract install dir, seems
REM like it must be run from there to find supporting files.
REM Tried adding it to PATH variable, etc. but that did not
REM work.
cd %TERMEXT_DIR%

REM run TermExt to create the report to be sent to CFM
termext.exe -flist %dir%filelist.txt -fo SGML -o %dir%TERMEXT_SGML.SUM -gensings -prefnp -nrdict %TERMEXT_DIR%\badterms.lx -minfreqcount 4 -sameminfreqcount 5 -nocaps -excldict %TERMEXT_DIR%\ibmterms.lx -uexcldict %TERMEXT_DIR%\genlex.lx -markterminology -showverbs -shortxmp %dir%TERMEXT_SGML_SHORT_CONTEXT.TXT -examples 3

REM return to source directory
cd %dir%

REM create .zip file to deliver to CFM
zip TERMEXT_TERMEXT.ZIP TERMEXT_SGML.SUM TERMEXT_SGML_SHORT_CONTEXT.TXT