#pragma once
// var __importStar = (this && this->__importStar) || function (mod) {
//     if (mod && mod.__esModule) return mod;
//     var result = {};
//     if (mod != null) for (var k in mod) if (Object.hasOwnProperty.call(mod, k)) result[k] = mod[k];
//     result["default"] = mod;
//     return result;
// };
// Object.defineProperty(exports, "__esModule", { value: true });
// const process = __importStar(require("process"));
// const path = __importStar(require("path"));
// const child_process_1 = require("child_process");
// const uuid_1 = require("uuid");
// const Logger_1 = require("./Logger");
#include "EnhancedEventEmitter.hpp"
#include "ortc.hpp"
#include "Channel.hpp"
#include "Router.hpp"

// // If env MEDIASOUP_WORKER_BIN is given, use it as worker binary.
// // Otherwise if env MEDIASOUP_BUILDTYPE is "Debug" use the Debug binary.
// // Otherwise use the Release binary.
// const workerBin = process.env.MEDIASOUP_WORKER_BIN
//     ? process.env.MEDIASOUP_WORKER_BIN
//     : process.env.MEDIASOUP_BUILDTYPE === "Debug"
//         ? path.join(__dirname, "..", "worker", "out", "Debug", "mediasoup-worker")
//         : path.join(__dirname, "..", "worker", "out", "Release", "mediasoup-worker");
// const logger = new Logger_1.Logger("Worker");
// const workerLogger = new Logger_1.Logger("Worker");

class Worker : public EnhancedEventEmitter {
    /**
     * @private
     * @emits died - (error: Error)
     * @emits @success
     * @emits @failure - (error: Error)
     */
	Worker({ logLevel, logTags, rtcMinPort, rtcMaxPort, dtlsCertificateFile, dtlsPrivateKeyFile, appData }) {
        super();
        // Closed flag.
        this->_closed = false;
        // Routers set.
        this->_routers = new Set();
        // Observer instance.
        this->_observer = new EnhancedEventEmitter();
        logger.debug("constructor()");
        let spawnBin = workerBin;
        let spawnArgs = [];
        if (process.env.MEDIASOUP_USE_VALGRIND === "true") {
            spawnBin = process.env.MEDIASOUP_VALGRIND_BIN || "valgrind";
            if (process.env.MEDIASOUP_VALGRIND_OPTIONS)
                spawnArgs = spawnArgs.concat(process.env.MEDIASOUP_VALGRIND_OPTIONS.split(/\s+/));
            spawnArgs.push(workerBin);
        }
        if (typeof logLevel === "string" && logLevel)
            spawnArgs.push(`--logLevel=${logLevel}`);
        for (const logTag of (Array.isArray(logTags) ? logTags : [])) {
            if (typeof logTag === "string" && logTag)
                spawnArgs.push(`--logTag=${logTag}`);
        }
        if (typeof rtcMinPort === "number" || !Number.isNaN(parseInt(rtcMinPort)))
            spawnArgs.push(`--rtcMinPort=${rtcMinPort}`);
        if (typeof rtcMaxPort === "number" || !Number.isNaN(parseInt(rtcMaxPort)))
            spawnArgs.push(`--rtcMaxPort=${rtcMaxPort}`);
        if (typeof dtlsCertificateFile === "string" && dtlsCertificateFile)
            spawnArgs.push(`--dtlsCertificateFile=${dtlsCertificateFile}`);
        if (typeof dtlsPrivateKeyFile === "string" && dtlsPrivateKeyFile)
            spawnArgs.push(`--dtlsPrivateKeyFile=${dtlsPrivateKeyFile}`);
        logger.debug("spawning worker process: %s %s", spawnBin, spawnArgs.join(" "));
        this->_child = child_process_1.spawn(
        // command
        spawnBin, 
        // args
        spawnArgs, 
        // options
        {
            env: {
                MEDIASOUP_VERSION: "3.5.9"
            },
            detached: false,
            // fd 0 (stdin)   : Just ignore it.
            // fd 1 (stdout)  : Pipe it for 3rd libraries that log their own stuff.
            // fd 2 (stderr)  : Same as stdout.
            // fd 3 (channel) : Producer Channel fd.
            // fd 4 (channel) : Consumer Channel fd.
            stdio: ["ignore", "pipe", "pipe", "pipe", "pipe"]
        });
        this->_pid = this->_child.pid;
        this->_channel = new Channel({
            producerSocket: this->_child.stdio[3],
            consumerSocket: this->_child.stdio[4],
            pid: this->_pid
        });
        this->_appData = appData;
        let spawnDone = false;
        // Listen for "running" notification.
        this->_channel.once(String(this->_pid), (event) => {
            if (!spawnDone && event === "running") {
                spawnDone = true;
                logger.debug("worker process running [pid:%s]", this->_pid);
                this->emit("@success");
            }
        });
        this->_child.on("exit", (code, signal) => {
            this->_child = undefined;
            this->close();
            if (!spawnDone) {
                spawnDone = true;
                if (code === 42) {
                    logger.error("worker process failed due to wrong settings [pid:%s]", this->_pid);
                    this->emit("@failure", new TypeError("wrong settings"));
                }
                else {
                    logger.error("worker process failed unexpectedly [pid:%s, code:%s, signal:%s]", this->_pid, code, signal);
                    this->emit("@failure", new Error(`[pid:${this->_pid}, code:${code}, signal:${signal}]`));
                }
            }
            else {
                logger.error("worker process died unexpectedly [pid:%s, code:%s, signal:%s]", this->_pid, code, signal);
                this->safeEmit("died", new Error(`[pid:${this->_pid}, code:${code}, signal:${signal}]`));
            }
        });
        this->_child.on("error", (error) => {
            this->_child = undefined;
            this->close();
            if (!spawnDone) {
                spawnDone = true;
                logger.error("worker process failed [pid:%s]: %s", this->_pid, error.message);
                this->emit("@failure", error);
            }
            else {
                logger.error("worker process error [pid:%s]: %s", this->_pid, error.message);
                this->safeEmit("died", error);
            }
        });
        // Be ready for 3rd party worker libraries logging to stdout.
        this->_child.stdout.on("data", (buffer) => {
            for (const line of buffer.toString("utf8").split("\n")) {
                if (line)
                    workerLogger.debug(`(stdout) ${line}`);
            }
        });
        // In case of a worker bug, mediasoup will log to stderr.
        this->_child.stderr.on("data", (buffer) => {
            for (const line of buffer.toString("utf8").split("\n")) {
                if (line)
                    workerLogger.error(`(stderr) ${line}`);
            }
        });
    }
    /**
     * Worker process identifier (PID).
     */
    get pid() {
        return this->_pid;
    }
    /**
     * Whether the Worker is closed.
     */
    get closed() {
        return this->_closed;
    }
    /**
     * App custom data.
     */
    get appData() {
        return this->_appData;
    }
    /**
     * Invalid setter.
     */
    set appData(appData) {
        throw new Error("cannot override appData object");
    }
    /**
     * Observer.
     *
     * @emits close
     * @emits newrouter - (router: Router)
     */
    get observer() {
        return this->_observer;
    }
    /**
     * Close the Worker.
     */
    close() {
        if (this->_closed)
            return;
        logger.debug("close()");
        this->_closed = true;
        // Kill the worker process.
        if (this->_child) {
            // Remove event listeners but leave a fake "error" hander to avoid
            // propagation.
            this->_child.removeAllListeners("exit");
            this->_child.removeAllListeners("error");
            this->_child.on("error", () => { });
            this->_child.kill("SIGTERM");
            this->_child = undefined;
        }
        // Close the Channel instance.
        this->_channel.close();
        // Close every Router.
        for (const router of this->_routers) {
            router.workerClosed();
        }
        this->_routers.clear();
        // Emit observer event.
        this->_observer.safeEmit("close");
    }
    /**
     * Dump Worker.
     */
    async dump() {
        logger.debug("dump()");
        return this->_channel.request("worker.dump");
    }
    /**
     * Get mediasoup-worker process resource usage.
     */
    async getResourceUsage() {
        logger.debug("getResourceUsage()");
        return this->_channel.request("worker.getResourceUsage");
    }
    /**
     * Update settings.
     */
    async updateSettings({ logLevel, logTags } = {}) {
        logger.debug("updateSettings()");
        const reqData = { logLevel, logTags };
        await this->_channel.request("worker.updateSettings", undefined, reqData);
    }
    /**
     * Create a Router.
     */
    async createRouter({ mediaCodecs, appData = {} } = {}) {
        logger.debug("createRouter()");
        if (appData && typeof appData !== "object")
            throw new TypeError("if given, appData must be an object");
        // This may throw.
        const rtpCapabilities = ortc.generateRouterRtpCapabilities(mediaCodecs);
        const internal = { routerId: uuid_1.v4() };
        await this->_channel.request("worker.createRouter", internal);
        const data = { rtpCapabilities };
        const router = new Router_1.Router({
            internal,
            data,
            channel: this->_channel,
            appData
        });
        this->_routers.add(router);
        router.on("@close", () => this->_routers.delete(router));
        // Emit observer event.
        this->_observer.safeEmit("newrouter", router);
        return router;
    }
};
