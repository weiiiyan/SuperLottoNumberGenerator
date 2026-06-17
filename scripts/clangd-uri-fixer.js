/**
 * clangd-uri-fixer.js — JSON-RPC proxy that fixes malformed file URIs
 *
 * Claude Code v2.1.179 on Windows sends Windows-style paths like
 *   file://c:/path  or  file://C:\path\to\file
 * instead of RFC 3986 compliant
 *   file:///C:/path/to/file
 *
 * This wrapper sits between Claude Code and clangd, intercepting
 * JSON-RPC messages and fixing URI fields before forwarding.
 *
 * Usage: node clangd-uri-fixer.js <path-to-real-clangd> [clangd-args...]
 */

const { spawn } = require('child_process');

const REAL_CLANGD = process.argv[2];
const CLANGD_ARGS = process.argv.slice(3);

if (!REAL_CLANGD) {
    console.error('Usage: node clangd-uri-fixer.js <path-to-clangd> [args...]');
    process.exit(1);
}

// ── Spawn real clangd ────────────────────────────────────────────────
const clangd = spawn(REAL_CLANGD, CLANGD_ARGS, {
    stdio: ['pipe', 'pipe', 'pipe'],
});

// Forward clangd stderr to our stderr for diagnostics
clangd.stderr.pipe(process.stderr);

// Log startup
process.stderr.write(`[clangd-uri-fixer] Proxying to: ${REAL_CLANGD} ${CLANGD_ARGS.join(' ')}\n`);

// ── URI fixing logic ─────────────────────────────────────────────────
function fixUri(uri) {
    if (typeof uri !== 'string') return uri;
    if (!uri.startsWith('file://')) return uri;

    let fixed = uri;

    // Fix: file://c:/...  →  file:///C:/...  (missing leading slash)
    // Also handles: file://C:/..., file://c:\..., file://C:\...
    fixed = fixed.replace(/^file:\/\/([a-zA-Z])[:\\]/ , (_, drive) =>
        `file:///${drive.toUpperCase()}:/`
    );

    // Normalize remaining backslashes to forward slashes
    fixed = fixed.replace(/\\/g, '/');

    return fixed;
}

// Deep-recursive fix of all URI fields in an object
function fixAllUris(obj) {
    if (obj === null || obj === undefined) return obj;
    if (typeof obj === 'string') return obj;

    if (Array.isArray(obj)) {
        for (let i = 0; i < obj.length; i++) {
            obj[i] = fixAllUris(obj[i]);
        }
        return obj;
    }

    if (typeof obj === 'object') {
        for (const key of Object.keys(obj)) {
            // Known URI fields in LSP protocol
            if (key === 'uri' || key === 'rootUri' || key === 'targetUri' ||
                key === 'documentUri' || key === 'oldUri' || key === 'newUri') {
                obj[key] = fixUri(obj[key]);
            } else {
                obj[key] = fixAllUris(obj[key]);
            }
        }
        return obj;
    }

    return obj;
}

// ── JSON-RPC message framing (Content-Length header) ──────────────────
let stdinBuffer = Buffer.alloc(0);

process.stdin.on('data', (chunk) => {
    stdinBuffer = Buffer.concat([stdinBuffer, chunk]);
    processBuffer();
});

function processBuffer() {
    while (stdinBuffer.length > 0) {
        const headerEnd = stdinBuffer.indexOf('\r\n\r\n');
        if (headerEnd === -1) return;

        const header = stdinBuffer.slice(0, headerEnd).toString();
        const contentLengthMatch = header.match(/Content-Length: (\d+)/i);
        if (!contentLengthMatch) {
            process.stderr.write('[clangd-uri-fixer] ERROR: No Content-Length header\n');
            return;
        }

        const contentLength = parseInt(contentLengthMatch[1], 10);
        const bodyStart = headerEnd + 4;
        const totalLength = bodyStart + contentLength;

        if (stdinBuffer.length < totalLength) return; // Need more data

        const body = stdinBuffer.slice(bodyStart, totalLength).toString();
        stdinBuffer = stdinBuffer.slice(totalLength);

        try {
            const msg = JSON.parse(body);
            const fixed = fixAllUris(msg);
            const fixedBody = JSON.stringify(fixed);
            const newHeader = `Content-Length: ${Buffer.byteLength(fixedBody)}\r\n\r\n`;
            clangd.stdin.write(newHeader + fixedBody);
        } catch (err) {
            // If parsing fails, pass through unchanged
            process.stderr.write(`[clangd-uri-fixer] JSON parse error, passing through: ${err.message}\n`);
            const newHeader = `Content-Length: ${Buffer.byteLength(body)}\r\n\r\n`;
            clangd.stdin.write(newHeader + body);
        }
    }
}

process.stdin.on('end', () => {
    clangd.stdin.end();
});

// ── Forward clangd responses back to Claude Code (unchanged) ──────────
clangd.stdout.pipe(process.stdout);

// ── Cleanup on exit ───────────────────────────────────────────────────
process.on('exit', () => {
    clangd.kill();
});
process.on('SIGINT', () => {
    clangd.kill();
    process.exit();
});
process.on('SIGTERM', () => {
    clangd.kill();
    process.exit();
});
