export { PackageCommand, PackageFlags, stringifyPackageCommand, MTU } from './package_define';
export { PackageEncoder, PackageEncryptOptions } from './encode';
export { PackageDecoder, PackageDecryptKeyDelegate, PackageDecryptOptions } from './decode';
export { TCPPackageEncoder } from './tcp_encode';
export { TCPPackageDecoder } from './tcp_decode';
export { PackageDispatcher, PackageHandlerPriority, HandlerDefination, IndexPackageDispatcher } from './dispatcher';
export { UDPPackageDecoder } from './udp_decode';
export { UDPPackageEncoder } from './udp_encode';

import { LoggerInstance } from '../base';
import { packageDefinations, fieldDictionary, packageFastIndex  } from './package_define';
import { PackageDispatcher, IndexPackageDispatcher } from './dispatcher';
import { PackageDecoder, PackageDecryptKeyDelegate } from './decode';
import { PackageEncoder } from './encode';
import { TCPPackageDecoder } from './tcp_decode';
import { TCPPackageEncoder } from './tcp_encode';
import { UDPPackageDecoder } from './udp_decode';
import { UDPPackageEncoder } from './udp_encode';
import { isNullOrUndefined } from 'util';

export function createPackageDispatcher(logger: LoggerInstance): PackageDispatcher {
    return new PackageDispatcher(logger, packageDefinations);
}

export function createTCPPackageEncoder(logger: LoggerInstance): TCPPackageEncoder {
    return new TCPPackageEncoder(logger, packageDefinations, fieldDictionary);
}

export function createTCPPackageDecoder(logger: LoggerInstance, keyDelegate?: PackageDecryptKeyDelegate): TCPPackageDecoder {
    return new TCPPackageDecoder(logger, packageDefinations, fieldDictionary, keyDelegate);
}

export function createUDPPackageDecoder(logger: LoggerInstance, keyDelegate?: PackageDecryptKeyDelegate): UDPPackageDecoder {
    return new UDPPackageDecoder(logger, packageDefinations, fieldDictionary, keyDelegate);
}

export function createUDPPackageEncoder(logger: LoggerInstance): UDPPackageEncoder {
    return new UDPPackageEncoder(logger, packageDefinations, fieldDictionary);
}

export function getPackageFastIndexKey(pkg: any): string | undefined {
    if (isNullOrUndefined(pkg.cmdType)) {
        return ;
    }

    let func = packageFastIndex.get(pkg.cmdType);
    if (!func) {
        return;
    }

    return func(pkg);
}

export function createIndexPackageDispatcher(logger: LoggerInstance): IndexPackageDispatcher {
    return new IndexPackageDispatcher(logger, packageDefinations);
}
