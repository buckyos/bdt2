import { transports, LoggerInstance, Logger } from 'winston';
export { LoggerInstance } from 'winston';
import * as path from 'path';
import * as fs from 'fs-extra';
import * as process from 'process';
const { LogShim } = require('./log_shim');

export type LoggerOptions = {
    logger?: LoggerInstance;
    loggerOptions?: {console: boolean, file?: {root: string, filename?: string}, level?: string};
};

export function initLogger(options: LoggerOptions): LoggerInstance {
    if (options.logger) {
        return options.logger;
    } else if (options.loggerOptions) {
        const loggerTransports = [];
        if (options.loggerOptions.console) {
            loggerTransports.push(new transports.Console({
                level: options.loggerOptions.level ? options.loggerOptions.level : 'info',
                timestamp: true,
                handleExceptions: true,
                humanReadableUnhandledException: true
            }));
        }
        if (options.loggerOptions.file) {
            fs.ensureDirSync(options.loggerOptions.file.root);
            loggerTransports.push(new transports.File({
                json: false,
                level: options.loggerOptions.level ? options.loggerOptions.level : 'info',
                timestamp: true,
                filename: path.join(options.loggerOptions.file.root, options.loggerOptions.file.filename || 'info.log'),
                datePattern: 'yyyy-MM-dd.',
                prepend: true,
                handleExceptions: true,
                humanReadableUnhandledException: true
            }));
        }
        const logger = new Logger({
            level: options.loggerOptions.level || 'info',
            transports: loggerTransports
        });
        
        return new LogShim(logger).log;
    } else {
        const loggerTransports = [];
        loggerTransports.push(new transports.Console({
            level: 'info',
            timestamp: true,
            handleExceptions: true
        }));
        const logger = new Logger({
            level: 'info',
            transports: loggerTransports
        });
        return new LogShim(logger).log;
    }
}

export function initUnhandledRejection(logger: LoggerInstance) {
    process.on('unhandledRejection', (_reason, p) => {
        const reason = _reason as {stack: any};
        console.log('Unhandled Rejection at: Promise ', p, ' reason: ', reason.stack);
        logger.error('Unhandled Rejection at: Promise ', p, ' reason: ', reason.stack);
        process.exit(-1);
    });
    
    process.on('uncaughtException', (err) => {
        console.log('uncaught exception at: ', err.stack);
        logger.error('uncaught exception at: ', err.stack);
        process.exit(-1);
    });    
}

export {LogShim};