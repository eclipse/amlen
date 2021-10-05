<?php
$filename = $_POST['FILENAME'];
$data = $_POST['DATA'];
if (($filename <> "") and ($data <> "")) {
	$uname = php_uname();
	$pos = strpos($uname, "Windows");
	if ($pos !== false) {
		$filename = "/cygwin/" . $filename;
	}
	$fp = fopen($filename, 'a') or die ('Can not open $filename');
	flock($fp,LOCK_EX);
	fwrite($fp,$data);
	fclose($fp);
	echo "$data";
}
?>