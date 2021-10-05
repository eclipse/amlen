<?php

//Read and return the whole file or return the value of the specified property
function myReadFile($filename, $property)
{
	$data = "";
	if ($property == NULL) {
		$fp = fopen($filename,'r');
		if ($fp == false) {
			// File does not exist, return empty string
			return $data; 
		}
		$data = fread($fp,filesize($filename));
		fclose($fp);
		return $data;
	}
	
	//Read the entire file into an array
	$lines = file($filename);
	if ($lines !== false) {
		while (list($i, $line) = each($lines)) {
			$pos = strpos($line, $property);
			if ($pos !== false) {
				//Found the property
				$arr = explode("=", $line, 2);
				if ($arr !== false) {
					//Found "=" and the value is the second element of the returned array
					$data = $arr[1];
				}
			}
		}
	}
	return $data;
}

//To unit test this php file, run php command line: 
// php readData.php <properties filename> <property> <timeout in milliseconds>
if (array_key_exists("FILENAME", $_GET)) {
	$filename = $_GET['FILENAME'];
} else {
	if ($argc > 1) {
		$filename = $argv[1];
	} else {
		die('Must specify a file to read from');
	}
}

$property = NULL;
if (array_key_exists("PROPERTY", $_GET)) {
	$property = $_GET['PROPERTY'];
} else {
	if ($argc > 2) {
		$property = $argv[2];
	}
}

//Timeout value in milliseconds
$mTimeout = 0;
if (array_key_exists("TIMEOUT", $_GET)) {
	$mTimeout = (int)$_GET['TIMEOUT'];
} else {
	if ($argc > 3) {
		$mTimeout = $argv[3];
	} else {
		$mTimeout = 10000; // Default timeout is 10 seconds
	}
}
$uTimeout = $mTimeout * 1000;
$uElapsedTime = 0;
$uTimeInterval = 1000;

$data = "";

while ($data == "") {
	$data = myReadFile($filename, $property);
	if ($data == "") {
		// Halt time in microseconds. A micro second is one millionth of a second.
		usleep($uTimeInterval);
		$uElapsedTime += $uTimeInterval;
		if ($uElapsedTime > $uTimeout) {
			break;
		}
	}
}
echo $data;

?>