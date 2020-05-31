const net = require('net');
import { Peerid, Endpoint, EndpointList, EndpointProtocol, EndpointIPVersion, PeerConstInfo, PeerMutableInfo, initLogger, initUnhandledRejection, ErrorCode, PeerPublicKeyType } from '../src/base';
import { NetworkManager, LocalPeer, PeerHistoryManager, SuperNodeServer} from '../src/framework';
import { PeerHistoryManagerImpl } from '../src/framework/peer_history';
import { PeerEstablishLog } from '../src/framework/peer_history';
import { createUDPPackageEncoder, createTCPPackageEncoder, PackageCommand } from '../src/protocol';
import * as Crypto from 'crypto';
import * as path from 'path';
import * as fs from 'fs';
import * as os from 'os';

// node simple_super_node_server.js -deviceid sn--server -udp 10020 -tcp 10030
listenCrash();

const workDir = path.dirname(__dirname);

const logger = initLogger({
    loggerOptions: {
        level: 'debug',
        file: {
            root: `${workDir}/logs`,
            filename: 'debug.log',
        },
        console: true
}});
initUnhandledRejection(logger);

type Command = {
    udp: number,
    tcp: number, // 可选
    deviceid: Buffer,
    ips: Array<string>, // 默认'0.0.0.0'
};

function parseParameters(): Command | undefined {
    let args = process.argv.slice(2);
    let argIndex = 0;
    let cmd: Command = {
        deviceid: Buffer.from('bucky-sn-server'),
        udp: 10020,
        tcp: 10030,
        ips: ['0.0.0.0', '::'],
    };

    try {
        while (argIndex < args.length) {
            switch (args[argIndex]) {
                case '-deviceid':
                    cmd.deviceid = Buffer.from(args[argIndex + 1]);
                    argIndex += 2;
                    break;
                case '-udp':
                    cmd.udp = parseInt(args[argIndex + 1]);
                    argIndex += 2;
                    break;
                case '-tcp':
                    cmd.tcp = parseInt(args[argIndex + 1]);
                    argIndex += 2;
                    break;
                case '-ip':
                    cmd.ips = JSON.parse(args[argIndex + 1]);
                    argIndex += 2;
                    break;
                default:
                    console.error(`Unknown argument: ${args[argIndex]}`);
                    return undefined;
            }
        }
    } catch (error) {
        console.error(`A error occurred when parse the arguments, detail:\n${error}`);
        return undefined;
    }

    let argsOk = true;
    if (cmd.deviceid.length === 0) {
        argsOk = false;
        console.error('You must specify the deviceid.');
    }
    if (!cmd.udp) {
        argsOk = false;
        console.error('You must specify the udp port to receive the packages from clients.');
    }

    if (argsOk) {
        return cmd;
    } else {
        return undefined;
    }
}

// 默认密钥对，一般需要指定，否则很危险
const DefaultKeyPair = {
    keyType: PeerPublicKeyType.RSA1024,
    publicKey: Buffer.from('30819f300d06092a864886f70d010101050003818d0030818902818100b7eb1058e858ed979be00ccda5e79bf73d232ed8a45f7c62ac794c6f671e17577ecfc5fad4bf1e0ae2540e91ba0b8062df3f9475e63c59be4b0f0c1256c0618b036a633ae85aa17d0e1c402e5473c7db779bb39f58db731b5978cbd90d2c0472cea155af23d9f880188e0a42f139dc0b4e56f9d813b3a0f3749bc39b6e8c31a50203010001', 'hex'),            
    secret: Buffer.from('3082025e02010002818100b7eb1058e858ed979be00ccda5e79bf73d232ed8a45f7c62ac794c6f671e17577ecfc5fad4bf1e0ae2540e91ba0b8062df3f9475e63c59be4b0f0c1256c0618b036a633ae85aa17d0e1c402e5473c7db779bb39f58db731b5978cbd90d2c0472cea155af23d9f880188e0a42f139dc0b4e56f9d813b3a0f3749bc39b6e8c31a502030100010281803002b8ddbca99a3c3d809b5703bc1646d03ae2fbc2ccfa5777d6a2516285c46a1ebc765e28334bd0638cb5d0ecd41bcbb3a39149c5b47368ed871c0b9d81d2f459c20e3103257ecb8f8961e90b7cc958487a995176d1fbcd5cb860f8f4b714372bd2315bedd0348cb3a51c9c0bc6ebcb3c719e564d9fda7f8115170e9452e171024100d9edcd5b8e1dc265667d518e1b91a555811c4ab11aeae260fca51e03ceb592b84b18ef018d59e51f1208e8700203234dd4f3847a5f7819f1285224186865deb7024100d80c3d006eabb1a589429d2fcb244320d7267f8e0d8676857164157b605710f81aa57914e8fa80f5c327ef1c02fb3b6e66f0bc0e563a5414faa2ecbad17496830241008cf569bcec818739bb3f17bf4949bd9d3eb3a404461ae36e443c30dbd99a4c5a74089e9f6c6456f4efdf5f2903c42fd3aa08110a6e31eae5b764da000796cca50241008a64c48acb59e671108d005dc636135e2d13f72f8ad07089a88a210ca838fda0c088f11808e9b6c437601456103ed8e22ec4d4e2263034fe3f53306bb792847b024100cfe6eb463fde203546aab93eeaba59b2fa31de343aa00308ea5ed97d68fa864803f895e77042c24ba364342131a9fe32dac4eac8ba0acde7ddf967e9a0a23b6e', 'hex'),    
};

async function run(cmd: Command) {

    function getLocalPeer(deviceId: Buffer) {
        // area code先写死
        let areaCode = {
            continent: 0,
            country: 0,
            carrier: 0,
            city: 0,
            inner: 0,
        };
        let constInfo = new PeerConstInfo();
        constInfo.fromObject({
            areaCode,
            deviceId,
            publicKeyType: DefaultKeyPair.keyType,
            publicKey: DefaultKeyPair.publicKey,
        });

        let mutableInfo: PeerMutableInfo = new PeerMutableInfo();
        for (let ip of cmd.ips) {
            let ipVersion = EndpointIPVersion.ipv4;
            if (net.isIPv6(ip)) {
                ipVersion = EndpointIPVersion.ipv6;
            }
            mutableInfo.endpointList.addEndpoint(new Endpoint({
                protocol: EndpointProtocol.udp,
                ipVersion,
                address: ip,
                port: cmd.udp,
            }));

            if (cmd.tcp) {
                mutableInfo.endpointList.addEndpoint(new Endpoint({
                    protocol: EndpointProtocol.tcp,
                    ipVersion,
                    address: ip,
                    port: cmd.tcp,
                }));
            }
        }

        let _localPeer = new LocalPeer({logger});
        _localPeer.init({
            constInfo,
            secret: DefaultKeyPair.secret,
            mutableInfo,
        });

        return _localPeer;
    }

    let localPeer = getLocalPeer(cmd.deviceid);

    let historyMgr = new PeerHistoryManagerImpl({logger, config: {keyExpire: 24 * 3600000, peerExpire: 24 * 3600000, logExpire: 24 * 3600000}});
    historyMgr.init({});

    let networkManager = new NetworkManager({logger});
    const networkInitRet = await networkManager.init({
        localPeer,
        historyManager: historyMgr,
    });
    if (networkInitRet.err) {
        console.error(`The program will exit for the network error:${networkInitRet.err}`);
        process.exit(networkInitRet.err);
        return networkInitRet.err;
    }

    let server = new SuperNodeServer({
        logger,
        config: {
            tcpFreeInterval: 10000,
            peerManager: {
                timeout: 120000,
                capacity: 4000,
            },
            resendQueue: {
                defaultInterval: 500,
                maxTimes: 3
            }
        }
    });

    const err = server.init({networkManager, encrypted: false});
    if (err) {
        console.error(`The program will exit for the SuperNode initialize failed:${err}`);
        return err;
    }

    console.info('sn started.');

    return 0;
}

function listenCrash() {
    function onCrash(err: any) {
        function saveToFile() {
            const errObj = {
                message: err.message,
                os: err.os,
                stack: err.stack,
            };
            const errJSON = JSON.stringify(errObj);
            let md5 = Crypto.createHash('md5');
            md5.update(errJSON);
            const hash = md5.digest().toString('hex');
            const now = new Date();
            const dirName = path.join(path.dirname(__dirname), 'crashs', hash);
            try {
                let crashsRoot = path.join(path.dirname(__dirname), 'crashs');
                try {
                    fs.mkdirSync(crashsRoot);
                } catch (error) {
                    console.error(error);
                }
                let classifyCrashDir = path.join(crashsRoot, hash);
                fs.mkdirSync(classifyCrashDir);
            } catch (error) {
                console.error(error);
            }

            const name = `${now.getFullYear()}-${now.getMonth()}-${now.getDate()}-${now.getHours()}-${now.getMinutes()}-${now.getSeconds()}-${now.getMilliseconds()}`;
            const fullPath = path.join(dirName, name);
            fs.writeFileSync(fullPath, errJSON, {encoding: 'utf8'});
            return fullPath;
        }

        // 启动上报进程
        const errFilePath = saveToFile();
    }

    process.on('unhandledRejection', (err) => onCrash(err));
    process.on('uncaughtException', (err) => onCrash(err));
}

function main() {
    const cmd: Command | undefined = parseParameters();
    
    if (cmd) {
        run(cmd);
    } else {
        console.log('Usage:');
        console.log('node simple_super_node_server.js -deviceid {Identify-for-the-device} -udp {udp-port-number} [-tcp {tcp-listen-port-number}] [-ip {listen-ip}]');
    }
}

main();