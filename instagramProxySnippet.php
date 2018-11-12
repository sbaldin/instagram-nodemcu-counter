<?php
$ch = curl_init();

/*$timezone  = 7; //(GMT -5:00) EST (U.S. & Canada) 
$lastPostTime = gmdate("H:i j.m", time() + 3600*($timezone+date("I"))); 
return "100" .";" .$lastPostTime .";". "10" .";". "1";*/
$host = "https://api.instagram.com/v1/users/self/?access_token=<INSERT THERE YOUR ACCESS TOKEN>";

$ch = curl_init();
curl_setopt($ch, CURLOPT_URL, $host);
curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1);
curl_setopt($ch, CURLOPT_CONNECTTIMEOUT, 300);
curl_setopt($ch,CURLOPT_FAILONERROR,true);
$data = curl_exec($ch);
$accountJson = json_decode($data, true);
$accountFollowedBy = $accountJson["data"]["counts"]["followed_by"];

$host = "https://api.instagram.com/v1/users/self/media/recent/?access_token=<INSERT THERE YOUR ACCESS TOKEN>";

$ch = curl_init();
curl_setopt($ch, CURLOPT_URL, $host);
curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1);
curl_setopt($ch, CURLOPT_CONNECTTIMEOUT, 300);
curl_setopt($ch,CURLOPT_FAILONERROR,true);
$data = curl_exec($ch);
curl_close($ch);

$accountJson = json_decode($data, true);
$createdTime = $accountJson["data"][0]["created_time"];
$lastPostLikes = $accountJson["data"][0]["likes"]["count"];
$lastPostComments = $accountJson["data"][0]["comments"]["count"];

$timezone  = 7; //(GMT -5:00) EST (U.S. & Canada) 
$lastPostTime = gmdate("H:i j.m", $createdTime + 3600*($timezone+date("I"))); 


$now = gmdate("H:i", time() + 3600*($timezone+date("I"))); 

return $accountFollowedBy .";" . $lastPostTime .";". $lastPostLikes .";". $lastPostComments. ";".$now;
