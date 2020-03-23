// import { Logger } from "./Logger";
// import { EnhancedEventEmitter } from "./EnhancedEventEmitter";
// import { Worker, WorkerSettings } from "./Worker";
// import * as utils from "./utils";
// import { supportedRtpCapabilities } from "./supportedRtpCapabilities";
// import { RtpCapabilities } from "./RtpParameters";
// import * as types from "./types";

/**
 * Expose all types.
 */
{ types };

/**
 * Expose mediasoup version.
 */
const version = "__MEDIASOUP_VERSION__";

/**
 * Expose parseScalabilityMode() function.
 */
{ parse as parseScalabilityMode } from "./scalabilityModes";

const Logger* logger = new Logger();
const observer = new EnhancedEventEmitter();

/**
 * Observer.
 *
 * @emits newworker - (worker: Worker)
 */
{ observer };

/**
 * Create a Worker.
 */
async function createWorker(
	{
		logLevel = "error",
		logTags,
		rtcMinPort = 10000,
		rtcMaxPort = 59999,
		dtlsCertificateFile,
		dtlsPrivateKeyFile,
		appData = {}
	}: WorkerSettings = {}
): Promise<Worker>
{
	logger.debug("createWorker()");

	if (appData && typeof appData !== "object")
		throw new TypeError("if given, appData must be an object");

	const worker = new Worker(
		{
			logLevel,
			logTags,
			rtcMinPort,
			rtcMaxPort,
			dtlsCertificateFile,
			dtlsPrivateKeyFile,
			appData
		});

	return new Promise((resolve, reject) =>
	{
		worker.on("@success", () =>
		{
			// Emit observer event.
			observer.safeEmit("newworker", worker);

			resolve(worker);
		});

		worker.on("@failure", reject);
	});
}

/**
 * Get a cloned copy of the mediasoup supported RTP capabilities.
 */
function getSupportedRtpCapabilities(): RtpCapabilities
{
	return utils.clone(supportedRtpCapabilities) as RtpCapabilities;
}
