import { isNullOrUndefined } from 'util';

export class SequenceCreator {
    constructor(options?: {
        cur?: number
    }) {
        if (options && !isNullOrUndefined(options.cur)) {
            this.m_curSequence = options.cur;
        } else {
            this.m_curSequence = Math.floor((1 << 20) * Math.random());
        }
    }

    newSequence(): number {
        this.m_curSequence += 1;
        if (this.m_curSequence >= (1 << 30)) {
            this.m_curSequence = 1;
        }
        return this.m_curSequence;
    }   

    clone(): SequenceCreator {
        let ins = new SequenceCreator({
            cur: this.m_curSequence
        });
        return ins;
    }

    get curSequence(): number {
        return this.m_curSequence;
    }

    private m_curSequence: number; 
}