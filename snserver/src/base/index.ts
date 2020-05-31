export * from './error_code';
export * from './logger';
export * from './crypto';
export * from './encode';
export * from './simple_command';
export * from './endpoint';
export * from './endpointlist';
export * from './peerid';
export * from './time';
export { BiMap } from './bimap';
export { BigNumber } from 'bignumber.js';

// TODO µ÷ÊÔÓÃ
let gloginfo: string[] = [];
export function addLoginfo(s: string) {
    gloginfo.push(`${new Date().toISOString()},${s}`);
    // console.log(`${new Date().toISOString()},${s}`);
}

export function printLog() {
    gloginfo.forEach((s) => {
        console.log(s);
    });
}
