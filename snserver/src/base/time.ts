import * as process from 'process';

export function processUptime(): number {
    return process.uptime() * 1000;
}

export function currentTime(): number {
    return Date.now();
}