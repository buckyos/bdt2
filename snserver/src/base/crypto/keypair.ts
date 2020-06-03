const secp256k1 = require('secp256k1');
const { randomBytes } = require('crypto');
import { isString } from 'util';

function signBufferMsg(msg: Buffer, key: Buffer) {
    // Sign message
    let sig = secp256k1.sign(msg, key);
    // Ensure low S value
    return secp256k1.signatureNormalize(sig.signature);
}

function verifyBufferMsg(msg: Buffer, sig: Buffer, key: Buffer): boolean {
    if (sig.length === 0) {
        return false;
    }

    if (key.length === 0) {
        return false;
    }

    try {
        sig = secp256k1.signatureNormalize(sig);
        return secp256k1.verify(msg, sig, key);
    } catch (e) {
        return false;
    }
}

const publicKeyLength: number = 33;
const secretLength: number = 64; 
const signatureLength: number = 64;

export {publicKeyLength, secretLength, signatureLength};

export function publicKeyFromSecretKey(secret: Buffer|string): Buffer | undefined {
    if (isString(secret)) {
        secret = Buffer.from(secret, 'hex');
    }
    if (!secp256k1.privateKeyVerify(secret)) {
        return;
    }
    const key = secp256k1.publicKeyCreate(secret, true);
    return key;
}

export function getECDHPublicKey(otherPublicKey: Buffer, secret: Buffer): Buffer {
    return secp256k1.ecdh(otherPublicKey, secret);
}

export function createKeyPair(): [Buffer, Buffer] {
    let privateKey;

    do {
        privateKey = randomBytes(32);
    } while (!secp256k1.privateKeyVerify(privateKey));

    const key = secp256k1.publicKeyCreate(privateKey, true);
    return [key, privateKey];
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
