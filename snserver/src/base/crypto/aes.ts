import * as crypto from 'crypto';

const default_iv = Buffer.alloc(16, 0);

function output(data: Buffer, cipher: crypto.Cipher|crypto.Decipher): Buffer {
    let buf1 = cipher.update(data);
    let buf2 = cipher.final();
    return Buffer.concat([buf1, buf2], buf1.length + buf2.length);
}

export function encryptData(key: Buffer, data: Buffer): Buffer {
    let cipher = crypto.createCipheriv('aes-256-cbc', key, default_iv);
    cipher.setAutoPadding(true);
    return output(data, cipher);
}

export function decryptData(key: Buffer, data: Buffer): Buffer {
    let decipher = crypto.createDecipheriv('aes-256-cbc', key, default_iv);
    decipher.setAutoPadding(true);
    return output(data, decipher);
}
