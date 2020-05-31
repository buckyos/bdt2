// 实现一个简单的BiMap，可以双向查找
import * as assert from 'assert';

export class BiMap<K, V> {
    private m_map: Map<K, V>;
    private m_inverseMap: Map<V, K>;
    constructor(entries?: ReadonlyArray<[K, V]>) {
        this.m_map = new Map();
        this.m_inverseMap = new Map();
        entries!.forEach((value) => {
            this.set(value[0], value[1]);
        });
    }

    set(key: K, value: V) {
        if (this.m_map.has(key) || this.m_inverseMap.has(value)) {
            assert(`add ${key} -> ${value} to map failed! check key or value`);
            return;
        }
        this.m_map.set(key, value);
        this.m_inverseMap.set(value, key);
    }

    key(key: K) {
        return this.m_map.get(key);
    }

    value(value: V) {
        return this.m_inverseMap.get(value);
    }
}