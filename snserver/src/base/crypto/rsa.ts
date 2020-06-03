import { isString } from 'util';
const NodeRSA = require('node-rsa');

function signBufferMsg(msg: Buffer, key: Buffer) {
    let rsaKeyPair = new NodeRSA(key, 'private-der');
    rsaKeyPair.setOptions({encryptionScheme: 'pkcs1'});
    return rsaKeyPair.sign(msg);
}

function verifyBufferMsg(msg: Buffer, sig: Buffer, key: Buffer): boolean {
    if (sig.length === 0) {
        return false;
    }

    if (key.length === 0) {
        return false;
    }

    try {
        let rsaKeyPair = new NodeRSA(key, 'public-der');
        rsaKeyPair.setOptions({signingScheme: 'pkcs1-md5'});
        rsaKeyPair.setOptions({encryptionScheme: 'pkcs1'});
        return rsaKeyPair.verify(msg, sig);
    } catch (e) {
        return false;
    }
}

export function createKeyPair(bits: number = 1024): [Buffer, Buffer] {
    let rsaKeyPair = new NodeRSA({b: bits});
    rsaKeyPair.setOptions({encryptionScheme: 'pkcs1'});
    let sk = rsaKeyPair.exportKey('private-der');
    let pk = rsaKeyPair.exportKey('public-der');
    return [pk, sk];
}

export function getKeySize(key: Buffer, isPublic: boolean) {
    let rsaKeyPair = new NodeRSA(key, isPublic ? 'public-der' : 'private-der'); 
    return rsaKeyPair.getKeySize();
}

export function sign(md: Buffer|string, secret: Buffer|string): Buffer {
    if (isString(secret)) {
        secret = Buffer.from(secret, 'hex');
    }
    if (isString(md)) {
        md = Buffer.from(md, 'hex');
    }
    return signBufferMsg(md, secret);
}

export function verify(md: Buffer|string, signature: Buffer, publicKey: Buffer): boolean {
    if (isString(md)) {
        md = Buffer.from(md, 'hex');
    }
    return verifyBufferMsg(md, signature, publicKey);
}

export function encrypt(data: Buffer, key: Buffer) {
    let rsaKeyPair = new NodeRSA(key, 'public-der');
    rsaKeyPair.setOptions({encryptionScheme: 'pkcs1'});
    return rsaKeyPair.encrypt(data);
}

export function decrypt(data: Buffer, key: Buffer) {
    let rsaKeyPair = new NodeRSA(key, 'private-der');
    rsaKeyPair.setOptions({encryptionScheme: 'pkcs1'});
    return rsaKeyPair.decrypt(data);
}