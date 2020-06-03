import { LoggerInstance, ErrorCode } from '../base';
import { PackageFieldDefination } from './field_define';
import { isUndefined, isFunction } from 'util';
import { stringifyPackageCommand } from './package_define';
import { getPackageFastIndexKey, createPackageDispatcher } from '.';

export type HandlerDefination = {
    handler: (pkg: {cmdType: number}&any) => {handled?: boolean}, 
    index?: object,
};

type Handler = {
    def: HandlerDefination,
    description: string, 
    priority: number,
    cookie: string
};

export enum PackageHandlerPriority {
    low = 0, 
    normal = 10000, 
    high = 20000
}

export class PackageDispatcher {
    constructor(protected readonly logger: LoggerInstance, private readonly packageDefinations: Map<number, any>) {

    }

    addHandler(options: {
        handler: HandlerDefination, 
        description: string, 
        priority?: number, 
    }): {err: ErrorCode, cookie?: string} {
        const cookie = `${this.m_curCookie++}`;
        this.m_handlers.push({def: options.handler, description: options.description, priority: isUndefined(options.priority) ? PackageHandlerPriority.normal : options.priority, cookie});
        this.m_handlers = this.m_handlers.sort((left, right) => {
            return - (left.priority - right.priority);
        });
        return {err: ErrorCode.success, cookie};
    }

    removeHandler(options: {
        cookie: string
    }) {
        for (let ix = 0; ix < this.m_handlers.length; ++ix) {
            const stub = this.m_handlers[ix];
            if (stub.cookie === options.cookie) {
                this.logger.debug(`handler ${stub.description} has been removed from disptacher`);
                this.m_handlers.splice(ix, 1);
                return;
            }
        }
    }

    removeAllHandlers() {
        this.m_handlers = [];
    }

    // TODO: 可以不迭代，要建立索引
    dispatch(pkg: {cmdType: number}&any): {handled: boolean} {
        const handlers = this.m_handlers.slice(0);
        let misMatchIndex = '';
        for (const stub of handlers) {
            let match = true;
            if (stub.def.index) {
                const pkgDef = this.packageDefinations.get(pkg.cmdType);
                for (const [field, value] of Object.entries(stub.def.index)) {
                    if (isFunction(value)) {
                        match = value(pkg[field]);
                    } else {
                        if (pkgDef[field]) {
                            const cr = (pkgDef[field] as PackageFieldDefination).equal(pkg[field], value);
                            match = cr.ret!;
                        } else {
                            match = (pkg[field] === value);
                        }
                    }

                    if (!match) {
                        misMatchIndex = field;
                        break;
                    }
                }
            }

            if (match) {
                if (this.logger.level === 'debug') {
                    this.logger.debug(`package ${stringifyPackageCommand(pkg.cmdType)} dispatch to handler ${stub.description}`);
                }
                const ret = stub.def.handler(pkg);
                if (ret.handled) {
                    if (this.logger.level === 'debug') {
                        this.logger.debug(`package ${stringifyPackageCommand(pkg.cmdType)} handled`);
                    }
                    return {handled: true};
                }
            }
        }
        if (this.logger.level === 'debug') {
            this.logger.debug(`package ${stringifyPackageCommand(pkg.cmdType)} not handled, misMatch: ${misMatchIndex}`);
        }
        return {handled: false};
    }

    get handlers(): Handler[] {
        return this.m_handlers;
    }

    private m_curCookie = 0;
    private m_handlers: Handler[] = [];
}

export class IndexPackageDispatcher {
    constructor(protected readonly logger: LoggerInstance, private readonly packageDefinations: Map<number, any>) {

    }

    addHandler(options: {
        index: {cmdType: number} & any,
        handler: HandlerDefination, 
        description: string, 
    }): {err: ErrorCode, cookie?: string} {
        let key = getPackageFastIndexKey(options.index);
        if (!key) {
            return {err: ErrorCode.invalidParam};
        }

        let dispatchers = this.m_dispatchers.get(options.index.cmdType);
        if (!dispatchers) {
            dispatchers = new Map();
            this.m_dispatchers.set(options.index.cmdType, dispatchers);
        }
        let dispatcher = dispatchers.get(key);
        if (!dispatcher) {
            dispatcher = createPackageDispatcher(this.logger);
            dispatchers.set(key, dispatcher);
        }
        return dispatcher.addHandler({
            handler: options.handler,
            description: options.description
        });
    }

    removeHandler(options: {
        index: {cmdType: number} & any,
        cookie: string
    }) {
        let key = getPackageFastIndexKey(options.index);
        if (!key) {
            return;
        }

        let dispatchers = this.m_dispatchers.get(options.index.cmdType);
        if (!dispatchers) {
            return;
        }
        let dispatcher = dispatchers.get(key);
        if (!dispatcher) {
            return;
        }
        dispatcher.removeHandler(options);
        if (!dispatcher.handlers.length) {
            dispatchers.delete(key);
            if (!dispatchers.size) {
                this.m_dispatchers.delete(options.index.cmdType);
            }
        }
    }

    removeHandlerByCmdType(options: {cmdType: number}) {
        if (this.m_dispatchers.has(options.cmdType)) {
            this.m_dispatchers.delete(options.cmdType);
        }
    }

    removeAllHandlers() {
        this.m_dispatchers = new Map();
    }

    dispatch(pkg: {cmdType: number}&any): {handled: boolean} {
        if (!pkg.cmdType) {
            return {handled: false};
        }

        let dispatchers = this.m_dispatchers.get(pkg.cmdType);
        if (!dispatchers) {
            return {handled: false};
        }

        let key = getPackageFastIndexKey(pkg);
        if (!key) {
            return {handled: false};
        }
        let dispatcher = dispatchers.get(key);
        if (!dispatcher) {
            return {handled: false};
        }

        return dispatcher.dispatch(pkg);
    }

    private m_dispatchers: Map<number, Map<any, PackageDispatcher>> = new Map(); 
}
