<?php
$port = $_POST['PORT'];
$address = $_POST['ADDRESS'];
$request = $_POST['REQUEST'];
$result = -1;

if (($port <> "") and ($address <> "") and ($request <> "")) {
    $errcode = 0;
    $errmsg = "";
    $socket_client = stream_socket_client("tcp://".$address.":".$port,$errcode,$errmsg);
    if (($errcode <> 0 )) {
        $result="Error creating the socket client. Code: ".$errcode." Message: ".$errmsg;
    }
    else {
        // Add a space and the solution name "unknown" after the request type (Ex: '1 unknown varname')
        $request = $request[0]." unknown".substr($request,1);
        $request_len = strlen($request);
        // The first byte sent must be the length.
        // The SyncDriver is expecting this to be an Integer
        fwrite($socket_client,chr($request_len));
        fwrite($socket_client,$request);
        $result=fread($socket_client,1024);
        fwrite($socket_client,chr(strlen('quit')));
        fwrite($socket_client,'quit');
    }
}
echo $result;
?>