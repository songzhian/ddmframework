<?php

date_default_timezone_set('Asia/Shanghai');

define('ROOT_PATH',dirname(__FILE__));

ini_set("display_errors", 1);
error_reporting(E_ALL ^ E_NOTICE);


$request_params['uid']  = 123;

$groupName = 'com/dang/wireless';
$serviceName = 'user';
$serviceVersion = "1.1.0-beta";
$request = array (
        'group' =>  $groupName,
        'service' => $serviceName,
        'version' => $serviceVersion,
        'request' => $request_params 
);
$config = [
	'usec_time_out' => 200,
	'host' => "127.0.0.1",
	'port' => 9898
	];
$request_params = array(
				'group' => 'com/dang/wireless',
				'action' => 'user',
				'version' => '1.1.0-beta',
				'trace_id' => "123456789",
				'udid' => 123456
);
$groupName = $request_params ['group'];
$serviceName = $request_params ['action'];
$serviceVersion = $request_params ['version'];
$request = array (
	'group' => $groupName,
	'service' => $serviceName,
	'version' => $serviceVersion,
	'trace_id' => $traceId,
	'trace_span_id' => $traceSpanId,
	'request' => $request_params 
	);

//msgpack_pack 序列化
$req_msg = msgpack_pack($request);
//get_buffer_request_body protobuffer 的扩展封装
$info = socketSend(get_buffer_request_body($groupName, $serviceName, $serviceVersion, $req_msg), $config);

var_dump( $info);
function socketSend($data, $config)
{

	$usecTimeOut = $config['time_out'];
	$host = $config['host'];
	$port = $config['port'];
	$socket = socket_create(AF_INET, SOCK_STREAM, SOL_TCP); //使用TCP协议，数据流模式，IPv4

	socket_set_option($socket, SOL_SOCKET, SO_LINGER, array('l_onoff' => 1, 'l_linger' => 0));
	socket_set_option($socket, SOL_SOCKET, SO_SNDTIMEO, array("sec" => 0, "usec" => $timeOut));
	socket_set_option($socket, SOL_SOCKET, SO_REUSEADDR, 1);
	//连接server端
	$r = socket_connect($socket, $host, $port);
	if (!$r) {
	    return;
	}
	//向server端发送数据
	$send = socket_write($socket, $data);

	$info =  socket_read($socket, 100);
	//关闭socket
	socket_close($socket);

	return $info ;

}


