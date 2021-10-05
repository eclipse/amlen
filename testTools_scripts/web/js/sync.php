<?php
$data = $_POST['DATA'];
$result = -1;

if (($data <> "")) {
  $result=`bash -c "./requestSync.sh $data 2>&1"`; 
}
echo $result;
?>
