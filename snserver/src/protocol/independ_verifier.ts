import { ErrorCode, LoggerInstance, stringifyErrorCode, currentTime } from '../base';

export type PackageIndependVerifyContext = {
    currentTime: number, 
};

export class PackageIndependVerifier {
    constructor(
        private logger: LoggerInstance, 
        private verifiers: Map<number, (pkg: any, context: PackageIndependVerifyContext) => ErrorCode>) {
        
    }

    verify(pkg: any): ErrorCode {
        const verifier = this.verifiers.get(pkg.cmdType);
        if (!verifier) {
            this.logger.debug(`invalid package cmdType ${pkg.cmdType}`);
            return ErrorCode.invalidPkg;
        }
        let context = {
            currentTime: currentTime()
        };
        let err = verifier(pkg, context);
        if (!err) {
            this.logger.debug(`package independ verify failed for ${stringifyErrorCode(err)}`);
        }
        return err;
    }
}