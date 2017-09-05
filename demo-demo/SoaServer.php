<?php

date_default_timezone_set('Asia/Shanghai');

define('ROOT_PATH',dirname(__FILE__));

ini_set("display_errors", 1);
error_reporting(E_ALL ^ E_NOTICE);


define('SERVICES_CONFIG_FILE', ROOT_PATH . DIRECTORY_SEPARATOR . "services.json");


class SoaServer {
	private $serv;
	public function __construct() {
		$this->serv = new swoole_server(get_host(SERVICES_CONFIG_FILE), get_port(SERVICES_CONFIG_FILE));
		$this->serv->set(array(
			'worker_num' => 1,
			'daemonize' => false,
			'max_request' => 10000,
			'dispatch_mode' => 2,
			'package_max_length' => 81920,
			'open_length_check' => true,
			'package_length_offset' => 8, 
			'package_body_offset' => 16, 
			'package_length_type' => 'N',
		));

		$this->serv->on('Start', array(
			$this,
			'onStart',
		));
		$this->serv->on('Shutdown', array(
				$this,
				'onShutdown',
		));
		$this->serv->on('Connect', array(
			$this,
			'onConnect',
		));
		$this->serv->on('Receive', array(
			$this,
			'onReceive',
		));
		$this->serv->on('Close', array(
			$this,
			'onClose',
		));
		$this->serv->on('workerStart', array(
			$this,
			'onWorkerStart',
		));
	}
	public function start() {
		$this->serv->start();
	}
	public function onStart($serv) {
		$ret = register_service_by_file('', SERVICES_CONFIG_FILE);
		if($ret == -1){
			soa_info('zk services registered failed');
			$serv->shutdown();
			return;
		}
		soa_info('zk services registered successfully');
		scp_write_node_value(NODE_FILE);
		//write pid
		$pid = getmypid();
		$myfile = fopen(PID_FILE, "w") or die("Unable to open file!");
		fwrite($myfile, $pid . "");
		fclose($myfile);
		soa_info('SOA server started!');
	}
	function onShutdown($serv)
	{
		soa_info('SOA server stop ...');
		soa_info('zk services unregistered');
		soa_info('SOA server stopped!');
	}
	public function onConnect($serv, $fd, $from_id) {
		soa_info('connect');
	}
	public function onReceive(swoole_server $serv, $fd, $from_id, $data) {

		// parse request packet
		$crc = unpack("N", $data)[1];
		$identifier = unpack("N", substr($data, 4, 4))[1];
		$length = unpack("N", substr($data, 8, 4))[1];
		$type = unpack("C", substr($data, 12, 1))[1];


		// check crc
		$crc_req = crc32(substr($data, 4));
		if ($crc_req != $crc) {
			soa_error('crc check failed and close it');
			$serv->close($fd);
			return;
		}

		$body = substr($data, 16);


		// encode response packet
		$req = msgpack_unpack($body);
		$service_name = $req['service'];
		$service_version = $req['version'];
		$request_params = $req['request'];
		
		// print_r ( $resp );
        $resp = "userinfo:uid";
		$resp = msgpack_pack($resp);
		$resp_len = strlen($resp);

		$response = pack("N", $identifier);
		$response .= pack("N", $resp_len);
		$response .= pack("C", ($type | (1 << 7)));
		$response .= pack("CCC", 0, 0, 0);
		$response .= $resp;
		$response = pack("N", crc32($response)) . $response;

		//soa_trace('RESP=' . bin2hex($response));

		$serv->send($fd, $response);
	}
	public function onClose($serv, $fd, $from_id) {
		soa_info('Client ' . $fd . ' close connection');
	}
	public function onWorkerStart($serv, $worker_id) {
		soa_info('Worker ' . $worker_id . ' start');
	}
}

$server = new SoaServer();

$server->start();

/**
 * 服务器地址
 */
function get_host($service_config_file) {
	$json_string = file_get_contents ( $service_config_file );
	$obj = json_decode ( $json_string, true );
	return $obj ['host'];
}

/**
 * 服务器端口
 */
function get_port($service_config_file) {
	$json_string = file_get_contents ( $service_config_file );
	$obj = json_decode ( $json_string, true );
	return $obj ['port'];
}

function soa_info($message) {
	print_r( $message);
}
