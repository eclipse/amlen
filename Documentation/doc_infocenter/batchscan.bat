REM java -Xmx5g -jar %TOOLS_DIR%/w2008_64/ScanTool/latest/BatchScanTool/dccbatch.jar -c .\config.properties -o .\report.csv -f .\safos_build\dist\en\Infocenter\plugins\com.ibm.safos.doc_4.1

call transform -acc accessibility.dcsbuild -rl accessibility_report.csv

call run_termext.bat

call %TOOLS_DIR%/w2008_64/KcCrawler/latest/KcCrawler -r http://www-03preprod.ibm.com -k SSWMAJ_5.0.0.1 -l en