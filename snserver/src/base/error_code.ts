
export enum ErrorCode {
    success = 0,
    invalidParam = 1,
    outOfLimit = 2,
    tooSmall = 3,
    unknownPackage = 4,
    verifyFailed = 5,
    localBindFailed = 6,
    notInitialize = 7,
    sendFailed = 8,
    exception = 9,
    invalidPkg = 10,
    invalidState = 11,
    notFound = 12,
    pending = 13,
    packageOverMTU = 14,
    timeout = 15,
    noEndpoint = 16,
    invalidEndpoint = 17,
    duplicateEndpoint = 18,
    notEstablish = 19,
    outOfMemory = 20,
    canceled = 21, 
    expired = 22,
    parseError = 22,
    noResponce = 23,

    noImpl = 100,
    unknown = 200,
}

export function stringifyErrorCode(err: ErrorCode): string {
    const errorString = Object.assign(
        {},
        ...Object.entries(ErrorCode).map(([a, b]) => ({ [b]: a }))
    );

    return errorString[err] ||  'unknown error code';
}
