/**
@file	utils.js
File that contains utility methods for DOM-style operations
*/


///////////////////////////////////////////////////////////////////////
// BEGIN AdobeProperty
/**
A property class that allows you to Get() and Set() a value and Bind()
that value to DOM elements or arbitrary functions to automagically
have things happen when the property changes.
*/

function AdobeProperty(inValue)
{
	this.value = inValue;
}


AdobeProperty.prototype.Get = function()
{
	return this.value;
}


AdobeProperty.prototype.Set = function(inValue)
{
	if (this.value != inValue)
	{
		this.value = inValue;
		if (this.objects)
		{
			for (var n = 0; n < this.objects.length; n++)
			{
				this._BindNotify(this.objects[n]);
			}
		}
	}
}


AdobeProperty.prototype.Bind = function(inObject, inMethod)
{
	var bRet = false;

	if (inObject)
	{
		var bindRecord = { obj: inObject, method: inMethod };
		if (this._BindNotify(bindRecord))
		{
			if (!this.objects)
				this.objects = new Array();
			this.objects.push(bindRecord);
			
			// Input elements get bi-directional
			if (inObject.nodeType == 1 && (inObject.nodeName == "INPUT" || inObject.nodeName == "SELECT"))
			{
				var thisCB = this;
				// Should preserve any existing onchange...
				inObject.onchange = function() { thisCB.Set(this.value); };
			}
			bRet = true;
		}
	}

	return bRet;
}


AdobeProperty.prototype._BindNotify = function(inBindRecord)
{
	var bRet = false;

	if (!inBindRecord || !inBindRecord.obj)
		return bRet;

	// If the bound method is a function, use it as a filter for the value.
	var targetValue = this.value;
	if ("function" == typeof inBindRecord.method)
		targetValue = inBindRecord.method(this.value);

	switch (typeof inBindRecord.obj)
	{
		case "function":
			// Bound to a function, just call it with the property value
			inBindRecord.obj(targetValue);
			bRet = true;
			break;
		case "string":
			// Bound to a string, set the value
			inBindRecord.obj = targetValue;
			bRet = true;
			break;
		case "object":
			// Bound to an object, then we try a few different things
			// 0. An object with a valid method bound
			if ("string" == typeof inBindRecord.method && "function" == typeof inBindRecord.obj[inBindRecord.method])
			{
				inBindRecord.obj[inBindRecord.method](targetValue);
				bRet = true;
			}
			// 1. An element
			else if (1 == inBindRecord.obj.nodeType)
			{
				// For form fields, set the value
				if ("INPUT" == inBindRecord.obj.nodeName || "SELECT" == inBindRecord.obj.nodeName)
				{
					inBindRecord.obj.value = targetValue;
				}
				// Other elements, replace the content
				else
				{
					while (inBindRecord.obj.lastChild)
						inBindRecord.obj.removeChild(inBindRecord.obj.lastChild);
					inBindRecord.obj.appendChild(document.createTextNode(targetValue));
				}
				bRet = true;
			}
			// 2. An attribute or text node
			else if (2 == inBindRecord.obj.nodeType || 3 == inBindRecord.obj.nodeType)
			{
				inBindRecord.obj.nodeValue = targetValue;
				bRet = true;
			}
			// 3. An object with a Set() method
			else if (inBindRecord.obj.Set && "function" == typeof inBindRecord.obj.Set)
			{
				inBindRecord.obj.Set(targetValue);
				bRet = true;
			}
			// 4. An object with a value property
			else if (undefined != inBindRecord.obj.value && (typeof inBindRecord.obj.value == typeof targetValue))
			{
				inBindRecord.obj.value = targetValue;
				bRet = true;
			}
			// 5. Don't know what to do
			break;
		default:
			// Unhandled type
	}
	return bRet;
}
// END AdobeProperty
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
// BEGIN WizardAlert

/**
Modal alert widget.
*/
function WizardAlert(inContainerProxy)
{
	this.session = inContainerProxy;
	this.title = null;
	this.body = null;
	this.buttons = new Array();

	if (!this.alertTemplate)
	{
		var alertPathArray = new Array(this.session.GetResourcesPath(), "/common/alert/alert.html");
		var alertPath = _concatPaths(alertPathArray, this.session.GetDefaultProperties().platform);
		
		var alertContents = this.session.LoadFile(alertPath);
		this.alertTemplate = alertContents.data;
		
		// doctor up the root path to the CSS
		var cssURIArray =  new Array(this.session.GetResourcesPath(), "/common/alert");
		var cssURI = "file://" + _concatPaths(cssURIArray, this.session.GetDefaultProperties().platform);
		this.alertTemplate = ExpandTokens(this.alertTemplate, { CSSRoot: cssURI });		
	}

	if (!this.alertTemplate || this.alertTemplate.length <= 0)
	{
		throw "Unable to load alert.html";
	}
}


/**
Run (show) the alert.
*/
WizardAlert.prototype.Run = function()
{
	// Formulate a token map to fill in the alert template.
	var repMap = new Object();

	// Title and body
	if (this.session.localization)
		repMap.AlertWindowTitle = this.session.localization.GetString("locAlertWindowTitle", "Installer Alert");
	else
		repMap.AlertWindowTitle = "Installer Alert";
	repMap.AlertTitle = this.title;
	repMap.AlertBody = this.body;

	// Button text padding
	var buttonBuffer = " "; // they look like spaces, but they are Special

	// Buttons
	repMap.AlertButtons = "";
	for (var bi = 0; bi < this.buttons.length; bi++)
	{
		// Setup standard button attributes
		var attributes = {
			type: "button",
			value: buttonBuffer + this.buttons[bi].label + buttonBuffer,
			onclick: "window.external.UIExitDialog('" + this.buttons[bi].returnCode + "');",
			onfocus: "setFocus(this);"
		};

		var html = "<input";
		var attr = null;

		// Standard attributs
		for (attr in attributes)
		{
			html += " " + attr + "=\"" + attributes[attr] + "\"";
		}

		// User supplied attributes augment or override
		for (attr in this.buttons[bi].attributes)
		{
			html += " " + attr + "=\"" + this.buttons[bi].attributes[attr] + "\"";
		}

		html += "/>";
		repMap.AlertButtons += html;
	}

	return this.session.UIShowModalAlert(ExpandTokens(this.alertTemplate, repMap));
}


/**
Set the title.  This will be automatically wrapped in an <h1> but may contain other HTML markup.
*/
WizardAlert.prototype.SetTitle = function(inTitle)
{
	this.title = inTitle;
}


/**
Set the body.  This will automatically be wrapped in a <div> and should contain appropriate markup, for example <p> tags.
*/
WizardAlert.prototype.SetBody = function(inBody)
{
	this.body = inBody;
}


/**
Add an alert button.  Buttons are displayed left to right in the order added.  An optional map can be supplied to augment or override the attributes in the generated <input type="button" ... /> fragment.
*/
WizardAlert.prototype.AddButton = function(inButtonLabel, inReturnCode, inOptAttributes)
{
	this.buttons.push({ label: inButtonLabel, returnCode: inReturnCode, attributes: inOptAttributes });
}


/**
A class to generate standard alerts used in a couple different places.
*/
function StandardAlert(inContainerProxy, inLocalizationObj)
{
	this.containerProxy = inContainerProxy;
	this.localizationObj = inLocalizationObj;
}


/**
Utility string lookup method to simplify handling the case where we don't have a Localization object
handy.
*/
StandardAlert.prototype._LoadText = function(inStringID, inDefaultText, inPropertyMap)
{
	var locText = inDefaultText;
	if (null == inPropertyMap)
	{
		inPropertyMap = this.containerProxy.GetDefaultProperties();	
	}
	if (null != this.localizationObj)
	{
		locText = this.localizationObj.GetString(inStringID, inDefaultText, inPropertyMap);
	}
	return locText;
	
}

/**
Standard invalid user credentials alert.
*/
StandardAlert.prototype.InvalidUserCredentials = function()
{
	var a = new WizardAlert(this.containerProxy);
	a.SetTitle(this._LoadText("locInvalidUserCredentialsTitle", "Invalid User Credentials"));
	a.SetBody("<p>" + this._LoadText("locInvalidUserCredentialsBody", "You do not have sufficient security credentials to install this software.") + "<p>");
	a.AddButton(this._LoadText("locBtnQuit", "Quit"), "2");
	return a.Run();
}


/**
Standard Setup already running alert.
*/
StandardAlert.prototype.SetupAlreadyRunning = function()
{
	var a = new WizardAlert(this.containerProxy);
	a.SetTitle(this._LoadText("locSetupRunningTitle", "Setup Already Running"));
	a.SetBody("<p>" + this._LoadText("locSetupRunningBody", "You can only install one Adobe product at a time.  Please complete the other installation before attempting to install this product." + "</p>"));
	a.AddButton(this._LoadText("locBtnQuit", "Quit"), "2");
	return a.Run();
}


/**
Standard bootstrap failure alert.
*/
StandardAlert.prototype.BootstrapInstallFailure = function(inExitCode, inErrorArgs)
{
	var propMap = this.containerProxy.GetDefaultProperties();
	var errorText = "";
	if (inErrorArgs["MSIErrorText"])
		errorText += inErrorArgs["MSIErrorText"];
	if (inErrorArgs["ARKErrorText"])
		errorText += inErrorArgs["ARKErrorText"];
	
	var locTitle = this._LoadText("locBootstrapFailedTitle", "Setup Failed");
	var locBody = "<p>" + this._LoadText("locBootstrapFailedBody", "Setup failed to bootstrap.") + "</p>";
	if (errorText && errorText.length > 0)
	{
		var errorTextNode = document.createElement("p");
		errorTextNode.id = "alertErrorText";
		var errorText = document.createTextNode(errorText);
		errorTextNode.appendChild(errorText);
		locBody += errorTextNode.innerHTML;
	}

	var a = new WizardAlert(this.containerProxy);
	a.SetTitle(locTitle);
	a.SetBody(locBody);
	a.AddButton(this._LoadText("locBtnQuit", "Quit"), "2");
	return a.Run();
}

// END WizardAlert
///////////////////////////////////////////////////////////////////////


function FormatLogHeader(inHeaderText)
{
    return "______ " + inHeaderText + " ______"
}


/**
Compare two versions that are in a.b.c.d format.
@retval <0 	iff version1 < version2
@retval 0 	iff version1 == version2
@retval >0	iff version1 > version2
*/
function compareVersions(version1, version2)
{
	var versionDelimsRE = /[,.]/;
	var versionArray1 = version1.split(versionDelimsRE);
	var versionArray2 = version2.split(versionDelimsRE);
	
	var returnVal = 0;
	var maxComponents = (versionArray1.length < versionArray2.length) ? versionArray2.length : versionArray1.length;
	
	for (var i=0; i < maxComponents; ++i)
	{
		if (versionArray1.length < i)
		{
			versionArray1.push(0);	
		}
		if (versionArray2.length < i)
		{
			versionArray2.push(0);	
		}
		if (Number(versionArray1[i]) > Number(versionArray2[i]))
		{
			returnVal = 1;
		}
		else if (Number(versionArray1[i]) < Number(versionArray2[i]))
		{
			returnVal = -1;
		}			
		
		if (returnVal != 0)
		{
			break;	
		}
	}
	return returnVal;
}


function _getSupportedLanguagesArray(inContainerProxy)
{
	var sessionSupportedLanguages = new Array();
	var payloadMap = inContainerProxy.GetSessionData().payloadMap;
	
	var mapFamilyNameToLangs = new Object();
	for (var adobeCode in payloadMap)
	{
		var payload = payloadMap[adobeCode];
		var satisfiesObj = payloadMap[adobeCode].Satisfies;

		var keyPair = satisfiesObj.family + satisfiesObj.productName;
		var langs = mapFamilyNameToLangs[keyPair];
		if (langs == null)
		{
			langs = new Object();
		}
		// Update the langs object
		if (payload.isLanguageIndependent != null &&
			true == payload.isLanguageIndependent)
		{
			var allLangs = getAllSupportedLanguagesArray();
			for (var langIndex = 0; langIndex < allLangs.length; ++langIndex)
			{
				langs[allLangs[langIndex]] = 1;
			}
		}
		else
		{
			for (var langIndex = 0; langIndex < payload.Languages.length; ++langIndex)
			{
				langs[payload.Languages[langIndex]] = 1;
			}
		}
		mapFamilyNameToLangs[keyPair] = langs;
	}	

	var firstIteration = true;
	for (var keyPair in mapFamilyNameToLangs)
	{
		// Get all the languages and intersect that with the existing set
		if (firstIteration)
		{
			var supportedLangMap = mapFamilyNameToLangs[keyPair];
			for (eachSupportedLang in supportedLangMap)
			{
				sessionSupportedLanguages.push(eachSupportedLang);
			}
			firstIteration = false;	
		}
		else
		{
			if (sessionSupportedLanguages.length <= 0)
			{
				break;	
			}
			// Remove any element that doesn't exist
			var updatedSupportedLangs = new Array();
			for (var supportedIndex = 0; supportedIndex < sessionSupportedLanguages.length; ++supportedIndex)
			{
				if (mapFamilyNameToLangs[keyPair][sessionSupportedLanguages[supportedIndex]] != null)
				{
					updatedSupportedLangs.push(sessionSupportedLanguages[supportedIndex]);
				}
			}
			sessionSupportedLanguages = updatedSupportedLangs;
		}		
	}
	
	return sessionSupportedLanguages;
}

/**
Test to see if this install supports only a single language
@retval "" 		the installer supports more locales or none
@retval string	the installer supports locale <string>
*/
function _extendScriptGetSingleLanguage()
{	
	var returnVal = "";
	
	try
	{
		var sendContainerProxy = new ContainerProxy;
		
		if (sendContainerProxy)
		{
			var langArray = _getSupportedLanguagesArray(sendContainerProxy);
	
			if (1 == langArray.length)
				returnVal = langArray[0];
		}
	}
	catch (ex)
	{
	} 
	
	return returnVal;
}

/**
Given an InstallerSession instance, traverse the payloads and determine how much disk space is required on each volume.
*/
function _calculateRequiredSpace(inInstallerSession)
{
	var mapRootToSize = inInstallerSession.OperationSize().roots;

	var mapVolumeToSize = new Object();
	var dirTokenMap = inInstallerSession.GetDirectoryTokenMap();
	dirTokenMap["[INSTALLDIR]"] = inInstallerSession.properties["INSTALLDIR"];
	for (var eachRoot in mapRootToSize)
	{
		var fullPath = dirTokenMap[eachRoot];
		if (null == fullPath)
		{
			throw "Unsupported directory token: " + eachRoot;
		}
		var volumeInfo = inInstallerSession.GetVolumeStatisticsFromPath(fullPath);
		if (!volumeInfo || !volumeInfo.rootPath)
		{
			throw "Unable to get volume statistics for path: " + fullPath;
		}
		var currentSize = mapVolumeToSize[volumeInfo.rootPath];
		if (null == currentSize)
		{
			currentSize = 0;
		}
		currentSize += mapRootToSize[eachRoot];
		mapVolumeToSize[volumeInfo.rootPath] = currentSize;
	}
		
	return mapVolumeToSize;
}


/**
Object that contains the status information for the current install operation.
*/
function InstallOperationStatus(inInstallerSession)
{
	/**  Current session */
	this.installerSession = inInstallerSession;
	/** The InstallerPayload that we're currently operating on */
	this.currentPayloadObj = null;
	/** Total number of operations that are going to be performed */
	this.totalOperations = 0;
	/** Normalized progress 0 <= n <= 100 */
	this.totalProgress = 0;
	/** Current operation index.  [0, totalOperations) */
	this.currentOperation = 0;
	/** Current relative disk index, from the set of media we are installing right now */
	this.currentRelativeDiskIndex = 0;
	/** Current media name, if any */	
	this.currentMediaName = null;
	/** Number of payloads for this disk that have been installed */	
	this.mediaPayloadsInstalled = 0;
	/** Total media count */
	this.mediaCount = 0;
	/** System Info */
	this.systemInfo = inInstallerSession.GetSystemInfo();	
	/** Last known media path **/
	this.lastMediaPath = null;
	/** Boolean to see if we can move on in media check loop **/
	this.foundOrCancelled = false;
	
	/** Current status object.  The status object has the following form:
	obj.message		
	--- All operation types --
	obj.message.code							Integer.  0 == success.  Other values are context sensitive.  See PayloadOperationConstants.h 
	--- InstallPayload Data ---
	obj.isRunning							Integer [0, 1]
	obj.percentComplete						Integer [0 - 100]
	obj.message.args						Array of arguments that are associated with the current obj.message.code value	
	--- Simulation Data ---
	obj.message.simulateResults						Anonymous object scoping payload results from CAPS simulation test
	obj.message.simulationResults.conflicting		Array of AdobeCodes that would be in conflict were this operation successful
	obj.message.simulationResults.upgraded			Array of AdobeCodes that would be upgraded were this operation successful
	obj.message.simulationResults.corrupted			Array of AdobeCodes that would be orphaned if this operation were successful.  Bootstrapper specific
	*/
	this.operationStatus = null;
	/** Flag that's set if the last operation encountered an error and
		we need to release from the progress since the operation
		won't necessarily have a percentComplete value */
	this.operationsComplete = false;
	this.percentPayload = null;
	/** Flag indicating that we should exit the workflow because the user selected cancel */
	this.exitOperationsLoop = false;
	
	this.updateProgress = function()
	{
		this.operationStatus = this.installerSession.GetInstallStatus();
		if (this.operationStatus != null)
		{			
			if (this.operationStatus.percentComplete != null)
			{
				this.percentPayload = this.operationStatus.percentComplete;
				if (this.totalOperations > 0)
				{
					if (100 == this.operationStatus.percentComplete &&
						this.currentOperation == (this.totalOperations-1))
					{
						this.totalProgress = 100;
					}
					else
					{
						var intervalSize = 1;
						if (this.totalOperations != 0)
							intervalSize = 100/this.totalOperations;
						if (intervalSize < 1)
							intervalSize = 1;

						var offset = (this.currentOperation)*intervalSize;
						offset += ((this.operationStatus.percentComplete * intervalSize)/100);
						this.totalProgress = offset;
					}
				}
			}
		}
		//this.installerSession.LogDebug("totalProgress: " + this.totalProgress);
	}
	this.isCurrentOperationComplete = function()
	{
		var isComplete = false;
		if (this.operationStatus &&
			this.operationStatus.isRunning == 0)
		{
			isComplete = true;
		}
		return isComplete;
	}
	
	this.areAllOperationsComplete = function()
	{
		var allComplete = this.operationsComplete;
		if (!allComplete)
		{
			if (this.totalOperations == (this.currentOperation+1))
			{
				allComplete = this.isCurrentOperationComplete();
			}
		}			
		return allComplete;
	}
}

function InstallOperationsQueue(inInstallerSession, inStatusCallback, inCancelCallback)
{
	this.installerSession = inInstallerSession;
	this.callbackMethod = inStatusCallback;
	this.cancelCallback = inCancelCallback;
	this.installOperationStatus = new InstallOperationStatus(inInstallerSession);
	this._operationsQueue = null;
	this.estimatedSize = 0;
	this.reverse = false; // True if we are uninstalling.

	this.Open = function()
	{
		var didOpen = false;
		if (null == this._operationsQueue)
		{
			this._operationsQueue = new Array();


			// Walk through the session payloads collecting ones with an interesting action.
			// But we don't allow mixed installs and removes as they are a little too interesting.
			var installCount = 0;
			var removeCount = 0;
			LogPayloadSet(this.installerSession, "InstallOperationsQueue Unordered operations", this.installerSession.sessionPayloads,
				function(p) { return "with operation " + p.GetInstallerAction(); });
			for (var adobeCode in this.installerSession.sessionPayloads)
			{
				var payload = this.installerSession.sessionPayloads[adobeCode];
				payload.SetOperationResult(null);
				switch (payload.GetInstallerAction())
				{
					case kInstallerActionInstall:
					case kInstallerActionRepair:
						installCount++;
						this._operationsQueue.push(payload);
						
						var opSize = payload.OperationSize(true);
						if (opSize && opSize.totalBytes)
						{
							var addSize = (opSize.totalBytes/1024);
							if (addSize < 1)
								addSize = 1;
							this.estimatedSize += Math.ceil(addSize);
						}
						break;
					case kInstallerActionRemove:
						removeCount++;
						this._operationsQueue.push(payload);
						break;
				}
			}
			
			if (installCount > 0 && removeCount > 0)
			{
				this.installerSession.LogError("InstallOperationsQueue: Installing and removing payloads in the same run is not supported");
			}
			else if (this._operationsQueue.length > 0)
			{
				// We have somthing to do, sort the queue
				this._operationsQueue.sort(PayloadSortOperationOrder);
				if (removeCount > 0)
				{
					this.reverse = true;
					this._operationsQueue.reverse();
				}

				LogPayloadSet(this.installerSession, "InstallOperationsQueue Ordered operations", this._operationsQueue,
					function(p) { return "with operation " + p.GetInstallerAction(); });

				// Negative one so we can PopInstruction and start with 0
				this.installOperationStatus.currentOperation = -1;
				this.installOperationStatus.totalOperations = this._operationsQueue.length;				

				didOpen = true;
			}
			else
			{
				this.installerSession.LogError("InstallOperationsQueue: Instruction set is empty");
			}
		}
		return didOpen;
	}

	
	this.PopInstruction = function(inIntervalCallback)
	{
		var retValue = null;
		while (retValue == null 
				&& (this.installOperationStatus.currentOperation+1)  < this.installOperationStatus.totalOperations)
		{
			this.installOperationStatus.currentOperation += 1;
			this.installerSession.LogDebug("Testing operation: " + this.installOperationStatus.currentOperation);			
			retValue = this._operationsQueue[this.installOperationStatus.currentOperation];
			
			// If all the dependencies are satisfied, then return it, otherwise
			// propagate the error and skip to the next one.
			this.installerSession.LogDebug("Popping instruction for payload " + retValue.LogID());
			
			// Test if the user canceled.  Note: this may also be
			// set via this.callbackMethod which is used in the operation status polling to pickup
			// a cancel in the middle of an operation.  Both methods have a site effect of setting
			// the exitOperationsLoop property of the installOperationStatus object.
			
			if (this.cancelCallback && "function" == typeof this.cancelCallback)
			{
				this.cancelCallback(this.installOperationStatus);
			}

			// If the user canceled then we need to short circuit
			if (this.installOperationStatus.exitOperationsLoop == true)
			{
				// Set the install status for all remaining payloads to complete
				this.installerSession.LogDebug("User exited workflow.");
				if (retValue)
				{				
					this.installerSession.LogDebug("Marking payload " + retValue.LogID() + " as canceled");
					this.installOperationStatus.operationStatus = new Object();
					this.installOperationStatus.operationStatus.isRunning = false;
					this.installOperationStatus.operationStatus.percentComplete = 100;
					this.installOperationStatus.operationStatus.message = new Object();
					this.installOperationStatus.operationStatus.message.code = 1; // OR_UserCancel
										
					this.installerSession.LogDebug("Status object");
					this.installerSession.LogDebug(this.installOperationStatus.operationStatus);
					
					var aPayload = this.installerSession.sessionPayloads[retValue.GetAdobeCode()];
					if (aPayload != null)
					{
						aPayload.SetOperationResult(this.installOperationStatus.operationStatus);
						this.installerSession.LogDebug("Set payload canceled status");
					}
					else
					{
						this.installerSession.LogDebug("Unable to get payload to set status");
					}
				}
				else
				{
					this.installerSession.LogDebug("Current operation is null");
				}
				retValue = null;				
			}
			else
			{
				// Make sure that whatever we needed has already happened...
				// But restrect checks to payloads in our operation queue.  The full requirement
				// set should have been validated upstream.
				var requiredOpsArray = this.reverse ? retValue.GetSatisfiedArray() : retValue.GetRequiredArray();
				requiredOpsArray = PayloadIntersect(requiredOpsArray, this._operationsQueue);

				LogPayloadSet(this.installerSession, "Dependent operations for " + retValue.LogID(), requiredOpsArray);

				for (var i =0; i < requiredOpsArray.length; ++i)
				{
					var aPayload = requiredOpsArray[i];
					this.installerSession.LogDebug("Checking operation result for " + aPayload.LogID());
					var operationResult = aPayload.GetOperationResult();
					if (null == operationResult)
					{
						this.installerSession.LogDebug("Required operation status is null, skipping current operation");
						// So clearly we're null as well
						retValue = null;
						break; 
					}
					else if (operationResult && operationResult._error)
					{
						
					}
					else if (operationResult)
					{
						// This operation should be complete.  If it is, and there's
						// an error, make that our result as well
						if (operationResult.message && operationResult.message.code)
						{
							if (operationResult.percentComplete >= 100)
							{
								if (operationResult.message &&
									!this.installerSession.IsOperationCodeSuccess(operationResult.message.code))
								{
									this.installerSession.LogError("Skipping operation because required operation failed with code: " + operationResult.message.code);
									if (retValue != null)
									{
										this.installerSession.LogError("Setting dependent operation result");
										retValue.SetOperationResult(operationResult);

										// If this is the last operation we need to set the special flag
										if (this.installOperationStatus.currentOperation+1 == this.installOperationStatus.totalOperations)
										{											
											this.installOperationStatus.operationsComplete = true;	
										}
									}									
									retValue = null;
									break;
								}
							}
						}
					}
					else
					{
						// Bad error somewhere...
						this.installerSession.LogError("Skipping operation because required operation failed with error: " + status.operationStatus._error);
						retValue.SetOperationResult(null);
						retValue = null;
						break;
					}
				}
			}
						
			this.installOperationStatus.currentPayloadObj = retValue;
		}
		return retValue;
	}
}

function _simulatePayloadOperations(inInstallerSession)
{
	var didVerify = false;
	var preflightQueue = new InstallOperationsQueue(inInstallerSession);
	if (preflightQueue.Open())
	{
		try
		{		
			var openSession = inInstallerSession.OpenCAPSSimulation();
			if (openSession.success)
			{
				var poppedInstruction = null;
				while (null != (poppedInstruction = preflightQueue.PopInstruction()))
				{
					// We just do this to the information that
					// is available in the GetOperationResult is the same between the simulation
					// and the thread-based model
					var installOperationStatus = new InstallOperationStatus(inInstallerSession);
					installOperationStatus.currentPayloadObj = poppedInstruction;
					installOperationStatus.operationStatus = inInstallerSession.SimulateInstallPayload(poppedInstruction.GetAdobeCode(), poppedInstruction.GetInstallerAction(), inInstallerSession.properties);
					poppedInstruction.SetOperationResult(installOperationStatus.operationStatus);
				}
				inInstallerSession.CloseCAPSSimulation();
			}	
		}
		catch (ex)
		{
			didVerify = false;
		}		
	}
	return didVerify;	
}

/**
Method to sequence and execute the individual payloads in the current session.
@param	inInstallerSession		Existing installer session instance
@param	inStatusCallback 		Callback method to invoke with status object
@param	inCancelCallback		Callback method returning true to continue operations, false to stop
*/
var _gInstallOperationsQueue = null;
var _gPollingIntervalName = null;

function _doPayloadOperations(inInstallerSession, inStatusCallback, inCancelCallback)
{
	// Get the total number of operations.
	try
	{
		_gInstallOperationsQueue = new InstallOperationsQueue(inInstallerSession, inStatusCallback, inCancelCallback);
		if (_gInstallOperationsQueue.Open())
		{
			inInstallerSession.LogDebug("Opened installation queue");
			var poppedInstruction = _gInstallOperationsQueue.PopInstruction();
			
			if (poppedInstruction)
			{
				// Set the media information in the property map
				var payloadObject = inInstallerSession.payloadMap[poppedInstruction.GetAdobeCode()];

				if (payloadObject && payloadObject.MediaInfo)
				{
					_gInstallOperationsQueue.installOperationStatus.lastMediaPath = inInstallerSession.properties["mediaPath"];
					inInstallerSession.properties["mediaType"] = payloadObject.MediaInfo.type; 
					inInstallerSession.properties["mediaVolumeIndex"] = payloadObject.MediaInfo.volumeIndex; 
					inInstallerSession.properties["mediaPath"] = payloadObject.MediaInfo.path; 
					inInstallerSession.properties["mediaName"] = payloadObject.MediaInfo.mediaName; 
					
					_gInstallOperationsQueue.installOperationStatus.currentRelativeDiskIndex = 1;
					_gInstallOperationsQueue.installOperationStatus.currentMediaName = payloadObject.MediaInfo.mediaName;
					_gInstallOperationsQueue.installOperationStatus.mediaPayloadsInstalled = 0;
					
					// See if we need to swap and prompt if we do
					DetectSwap(payloadObject, poppedInstruction.GetInstallerAction());
				}
				else
				{
					inInstallerSession.properties["mediaType"] = "0"; 
					inInstallerSession.properties["mediaVolumeIndex"] = "1";
					inInstallerSession.properties["mediaPath"] = ""; 
					inInstallerSession.properties["mediaName"] = "";
					
					_gInstallOperationsQueue.installOperationStatus.currentRelativeDiskIndex = 1;
				}
				
				// Get all the payloads with an install operation and sum them.  This is the "AddRemoveEstimatedSize" value
				inInstallerSession.properties["AddRemoveInfoEstimatedSize"] = _gInstallOperationsQueue.estimatedSize;
				
				// We'll get the rest of it in the callback method
				inInstallerSession.InstallPayload(poppedInstruction.GetAdobeCode(), poppedInstruction.GetInstallerAction(), inInstallerSession.properties);
				if(inInstallerSession.UIHosted())
				{	
					_gInstallOperationsQueue.installerSession.LogInfo("Setting interval");		
					_gPollingIntervalName = window.setTimeout("_pollOperationStatus()", 200);
				}
				else
				{
					_gInstallOperationsQueue.installerSession.LogInfo("Setting callback for silent");		
					_gPollingIntervalName = "_pollOperationStatus()";
					eval("_pollOperationStatus()");
				}
			}
		}
		else
		{
			throw "Unable to open operation queue";
		}	
	}
	catch (ex)
	{
		if (inInstallerSession)
			inInstallerSession.LogError("error creating instructions: " + ex);
	}
}

function _pollOperationStatus()
{	
	try
	{
		if (!_gInstallOperationsQueue)
		{
			return;
		}
		
		if (_gPollingIntervalName != null && _gInstallOperationsQueue.installerSession.UIHosted())
		{	
			//_gInstallOperationsQueue.installerSession.LogDebug("Destroying interval: " + _gPollingIntervalName);
			//clearInterval(_gPollingIntervalName);
			_gPollingIntervalName = null;
		}
		else if (_gPollingIntervalName)
		{
			_gInstallOperationsQueue.installerSession.LogDebug("Destroying interval: " + _gPollingIntervalName);
			_gPollingIntervalName = null;
		}
	
		//_gInstallOperationsQueue.installerSession.LogDebug("updating progress");
		_gInstallOperationsQueue.installOperationStatus.updateProgress();
		try
		{
			_gInstallOperationsQueue.callbackMethod(_gInstallOperationsQueue.installOperationStatus);
		}
		catch (ex)
		{
			_gInstallOperationsQueue.installerSession.LogDebug("callbackMethod failed: " + ex);
		}
		
		if (_gInstallOperationsQueue.installOperationStatus.isCurrentOperationComplete() == true)
		{
			// If the current instruction has completed, we can pop the next ones
			// Save the current status
			if (_gInstallOperationsQueue.installOperationStatus.currentPayloadObj != null)
			{
				_gInstallOperationsQueue.installerSession.LogDebug("Operation complete. Setting status: " + _gInstallOperationsQueue.installOperationStatus.operationStatus.message.code);
				_gInstallOperationsQueue.installOperationStatus.currentPayloadObj.SetOperationResult(_gInstallOperationsQueue.installOperationStatus.operationStatus);
			}
			else
			{
				_gInstallOperationsQueue.installerSession.LogDebug("No status available for payload");				
			}
			_gInstallOperationsQueue.installerSession.LogDebug("Popping new operation");
			var poppedInstruction = _gInstallOperationsQueue.PopInstruction();
			_gInstallOperationsQueue.installerSession.LogDebug("Popped");
			if (poppedInstruction)
			{
				var payloadObject = _gInstallOperationsQueue.installerSession.payloadMap[poppedInstruction.GetAdobeCode()];
				
				if (payloadObject && payloadObject.MediaInfo)
				{
					_gInstallOperationsQueue.installOperationStatus.lastMediaPath = _gInstallOperationsQueue.installerSession.properties["mediaPath"];
					_gInstallOperationsQueue.installerSession.properties["mediaType"] = payloadObject.MediaInfo.type;
					_gInstallOperationsQueue.installerSession.properties["mediaVolumeIndex"] = payloadObject.MediaInfo.volumeIndex; 
					_gInstallOperationsQueue.installerSession.properties["mediaPath"] = payloadObject.MediaInfo.path; 
					_gInstallOperationsQueue.installerSession.properties["mediaName"] = payloadObject.MediaInfo.mediaName;
					
					if (_gInstallOperationsQueue.installOperationStatus.currentMediaName != payloadObject.MediaInfo.mediaName)
					{
						_gInstallOperationsQueue.installOperationStatus.mediaPayloadsInstalled = 0;
						
						if (_gInstallOperationsQueue.installOperationStatus.currentRelativeDiskIndex < _gInstallOperationsQueue.installOperationStatus.mediaCount)
							_gInstallOperationsQueue.installOperationStatus.currentRelativeDiskIndex += 1;
					}
					else
					{
						_gInstallOperationsQueue.installOperationStatus.mediaPayloadsInstalled += 1;
					}
					
					// See if we need to swap and prompt if we do
					DetectSwap(payloadObject, poppedInstruction.GetInstallerAction());
					
					_gInstallOperationsQueue.installOperationStatus.currentMediaName = payloadObject.MediaInfo.mediaName; 
				} 
				else
				{
					_gInstallOperationsQueue.installerSession.properties["mediaType"] = "0"; 
					_gInstallOperationsQueue.installerSession.properties["mediaVolumeIndex"] = "1";
					_gInstallOperationsQueue.installerSession.properties["mediaPath"] = ""; 
					_gInstallOperationsQueue.installerSession.properties["mediaName"] = "";					
				}

				// We'll ge the rest of it in the callback method
				if (_gInstallOperationsQueue.installOperationStatus.exitOperationsLoop != true)
				{
					_gInstallOperationsQueue.installerSession.InstallPayload(poppedInstruction.GetAdobeCode(), poppedInstruction.GetInstallerAction(), _gInstallOperationsQueue.installerSession.properties)
				}
				
				if (_gInstallOperationsQueue.installerSession.UIHosted())
				{
					_gInstallOperationsQueue.installerSession.LogInfo("setting timeout");
					_gPollingIntervalName = window.setTimeout("_pollOperationStatus()", 200);
					//_gInstallOperationsQueue.installerSession.LogDebug("Created interval name:" + _gPollingIntervalName);
				}
				else
				{
					_gInstallOperationsQueue.installerSession.LogInfo("Setting callback for silent");
					_gPollingIntervalName = "_pollOperationStatus()";
					eval("_pollOperationStatus()");
				}
			}
			else
			{
				// Couldn't pop.  Are we done?
				// _gInstallOperationsQueue.installOperationStatus.updateProgress();
				_gInstallOperationsQueue.installerSession.LogInfo("No operation.  We're done:");
				_gInstallOperationsQueue.installOperationStatus.operationsComplete = true;
				
				// Attempt to eject the media if we are installing from removable media
				if (_gInstallOperationsQueue.installOperationStatus && _gInstallOperationsQueue.installOperationStatus.lastMediaPath)
					_gInstallOperationsQueue.installerSession.UIEjectRemovableMedia(_gInstallOperationsQueue.installOperationStatus.lastMediaPath);
				
				// Invoke the callback
				_gInstallOperationsQueue.callbackMethod(_gInstallOperationsQueue.installOperationStatus);
			}
		}
		else
		{
			// Come around for another query
			if (_gInstallOperationsQueue.installerSession.UIHosted())
			{
				_gPollingIntervalName = window.setTimeout("_pollOperationStatus()", 200);
			}
			else
			{
				_gPollingIntervalName = "_pollOperationStatus()";
				eval("_pollOperationStatus()");
			}
		}	

	}
	catch (ex)
	{
		_gInstallOperationsQueue.installerSession.LogError("Polling error:" + ex);		
	}
}

/**
Property turn JSON string into JS object by encapsulating in parens.
@param	inJSONText	JSON text
@returns	Re-hydrated object, null if JSON invalid.
*/
function _jsonToObject(inJSONText)
{
	var retObj = null;
	try
	{
		retObj = eval('(' + inJSONText + ')');
	}
	catch (e)
	{
		retObj = null;
	}
	return retObj;	
}

/**
Concatenate two strings given the current platform string
@param	inPathComp		Array of paths to concatenate
@param	inPlatName		Platform name
@returns Concat'd paths
*/
function _concatPaths(inPathComp, inPlatName)
{
	
	var retString = "";
	var pathComp = "";
	var pathSep = '/';
	
	// If the first path component starts with a UNC marker then
	// we need to preserve this after normalization
	// 1442989
	var uncPath = false;

	if (inPathComp[0]) 
	{
		if (inPathComp[0].length > 1)
		{
			uncPath = (inPathComp[0].charAt(0) == '\\' && inPathComp[0].charAt(1) == '\\');
		}
		
		for (var i = 0; i < inPathComp.length; i++)
		{
			retString += (inPathComp[i] + "/");
		}
		
		// Convert all backslashes to forward slashes
		retString = retString.replace(/\\+/g, "/");
		if ("Win32" == inPlatName)
		{
			retString = retString.replace(/\/+/g, "\\"); 
			if (uncPath)
			{
				retString = '\\' + retString;
			}
		}
		else
		{
			retString = retString.replace(/[\/\:]+/g, "/"); 
		}
		if (retString.charAt(retString.length-1) == '\\' ||
			retString.charAt(retString.length-1) == '/')
		{
			retString = retString.slice(0, retString.length-1);
		}
	} 
	else 
	{
		throw "Invalid paths provided to _concatPaths";
	}

	return retString;
}


/**
Apply inFunction to inNode as well as all of inNode's children
@param	inNode 	DOM element
@param	inFunction Unary function taking DOMNode to apply to all elements
*/
function applyToDOMFragment(inNode, inFunction)
{
	inFunction(inNode);
	
	if (inNode.all)
	{
		for (var i = 0; i < inNode.all.length; ++i)
			inFunction(inNode.all[i]);
	}
	else if (null != inNode.getElementsByTagName)
	{
		var childNodes = inNode.getElementsByTagName('*');
		for (var i = 0; i < childNodes.length; ++i)
			inFunction(childNodes[i]);
	}
	else
	{
		var childNodes;
		
		if (inNode.childNodes != null)
			childNodes = inNode.childNodes;
		else if (inNode.children != null)
			childNodes = inNode.children;
		
		if (childNodes != null)
		{
			// make a copy of the node list so that manipulations of
			// the list will not affect the traversal
			var childNodesArray = new Array;
			for (var nodeIndex = 0; nodeIndex < childNodes.length; ++nodeIndex)
				childNodesArray.push(childNodes[nodeIndex]);
			
			// iterate node list copy
			for (var i = 0; i < childNodesArray.length; ++i)
				applyToDOMFragment(childNodesArray[i], inFunction);
		}
	}
}

function listProperties(obj, objName)
{
	var result = "";
	for (var i in obj)
	{
		result += objName + "." + i + "=" + obj[i] + "\n";
	}
	return result;
}


function printNodeTree(inNode, depth)
{
	rv = "";
	for (var i = 0; i < depth; i++)
		rv = rv + "  ";
	
	if (1 == inNode.nodeType)
	{
		rv = rv + inNode.nodeName;
		if (inNode.attributes)
		{
			rv = rv + ": ";
			for (var a = 0; a < inNode.attributes.length; a++)
			{
				rv = rv + inNode.attributes.item(n).nodeName + ": " + inNode.attributes.item(n).nodeValue;
			}
		}
	}
	else
	{
		rv = rv + inNode.nodeName + ": " + inNode.nodeValue;
	}

	rv = rv + "\n";

	for (var n = 0; inNode.childNodes && n < inNode.childNodes.length; n++)
	{
		rv = rv + printNodeTree(inNode.childNodes[n], depth + 1);
	}

	return rv;
}


function bytesToText(inBytes)
{
	var session = gSession;
	var units = [ 
		{ id: "locSizeKilobytes", format: "[size] KB" },
		{ id: "locSizeMegabytes", format: "[size] MB" },
		{ id: "locSizeGigabytes", format: "[size] GB" },
		{ id: "locSizeTerabytes", format: "[size] TB" }
	];
		
	var signStr = inBytes < 0 ? "-" : "";
	inBytes = Math.abs(inBytes);
	var unit = 0;
	while (unit < units.length)
	{
		var size = inBytes / Math.pow(1024, (unit + 1));
		if (size < 1000)
			break;
		unit++;
	}
	
	var decimal = ".";
	if (session && session.localization && session.UIHosted())
		decimal = session.localization.GetString("locDecimal", decimal);

	var sizeStr = new Number(Math.round(size * 10)).toString();
	if (sizeStr.length == 1)
		sizeStr = "0" + sizeStr;
	sizeStr = signStr + sizeStr.slice(0, -1) + decimal + sizeStr.slice(-1);

	var ls = new LocalizedString(units[unit].id, units[unit].format, { size: sizeStr });
	if (session && session.localization && session.UIHosted())
		return ls.Translate(session.localization);
	return ls.Translate(null);
}


//! Deletes all the descendants of the elementName.
//! \returns, the node of the with the Id elementName.
function RemoveAllChildren(element)
{
	if (element)
	{
		while (element && element.childNodes[0]) {
			element.removeChild(element.childNodes[0]);
		}
	}
	return element;
}


//! Deals with detecting if a disk swap is needed
function DetectSwap(payloadObject, inInstallerAction)
{
	// If removable, we prompt for the disk swap if we don't already see the disk
	if (1 == payloadObject.MediaInfo.type 
			&& _gInstallOperationsQueue.installerSession.UIHosted() 
			&& (kInstallerActionRemove != inInstallerAction))
	{
		_gInstallOperationsQueue.installOperationStatus.foundOrCancelled = false;
		var mediaPathInfo = _gInstallOperationsQueue.installerSession.GetPathInformation(payloadObject.MediaInfo.path);
		
		// Test to see if the disk needed is there, if not, prompt
		if (mediaPathInfo && mediaPathInfo.isValidPath && mediaPathInfo.isValidPath == 1 && mediaPathInfo.pathExists && mediaPathInfo.pathExists == 1)
		{
			_gInstallOperationsQueue.installOperationStatus.foundOrCancelled = true;
		}

		while (!_gInstallOperationsQueue.installOperationStatus.foundOrCancelled && gWizardControl && gWizardControl.pageUniverse && gWizardControl.pageUniverse[0])
		{
			// Attempt to eject the media in question
			if (_gInstallOperationsQueue.installOperationStatus.lastMediaPath)
				_gInstallOperationsQueue.installerSession.UIEjectRemovableMedia(_gInstallOperationsQueue.installOperationStatus.lastMediaPath);
			
			var alertObj = new WizardAlert(_gInstallOperationsQueue.installerSession);
			alertObj.SetTitle(_gInstallOperationsQueue.installerSession.localization.GetString("locProductName", "[productName]"));
			alertObj.SetBody("<p>" + _gInstallOperationsQueue.installerSession.localization.GetString("locAlertDiskSwapBody", "Please insert \"[mediaName]\" to continue installation.", { mediaName: payloadObject.MediaInfo.mediaName}) + "</p>");

			alertObj.AddButton(_gInstallOperationsQueue.installerSession.localization.GetString("locBtnQuit", "Cancel"), "1", { accessKey: "c" } );
			alertObj.AddButton(_gInstallOperationsQueue.installerSession.localization.GetString("locAlertDiskSwapOk", "OK"), "2", { accessKey: "o" } );
			
			var resultDialog = alertObj.Run();

			if (resultDialog && resultDialog.returnValue && resultDialog.returnValue == "2")
			{
				// Test to see if the disc is on the default removable drive 
				mediaPathInfo = _gInstallOperationsQueue.installerSession.GetPathInformation(payloadObject.MediaInfo.path);
				if (mediaPathInfo && mediaPathInfo.isValidPath && mediaPathInfo.isValidPath == 1 && mediaPathInfo.pathExists && mediaPathInfo.pathExists == 1)
				{
					_gInstallOperationsQueue.installOperationStatus.foundOrCancelled = true;
				}
				
				// Test to see if the disc is in any removable drive (Win)
				if (!_gInstallOperationsQueue.installOperationStatus.foundOrCancelled && _gInstallOperationsQueue.installOperationStatus.systemInfo && _gInstallOperationsQueue.installOperationStatus.systemInfo.RemovableDrives)
				{
					var removableDriveList = _gInstallOperationsQueue.installOperationStatus.systemInfo.RemovableDrives;
					var secondChoicePath = payloadObject.MediaInfo.path;
					var secondChoicePathInfo = null;
					
					if (removableDriveList)
					{
						for (var i = 0; i < removableDriveList.length; i++)
						{
							secondChoicePath = removableDriveList[i].root + secondChoicePath.slice(3, secondChoicePath.length);
							secondChoicePathInfo  = _gInstallOperationsQueue.installerSession.GetPathInformation(secondChoicePath);
							
							if (secondChoicePathInfo && secondChoicePathInfo.isValidPath && secondChoicePathInfo.isValidPath == 1 && secondChoicePathInfo.pathExists && secondChoicePathInfo.pathExists == 1)
							{ 
								_gInstallOperationsQueue.installerSession.properties["mediaPath"] = secondChoicePath;
								_gInstallOperationsQueue.installOperationStatus.foundOrCancelled = true;
								break;
							}
						}
					}
				}
			}
			else
			{
				_gInstallOperationsQueue.installOperationStatus.exitOperationsLoop = true;
				_gInstallOperationsQueue.installOperationStatus.foundOrCancelled = true;
			}
		}
	}
}


/**
Payload ordering according to dependency.

Order is calculated from the dependency graph:

 1. Minimum incoming (dependency) edges so those with no
    dependencies go down first.
 2. Maximum outgoing (satisfying) edges so that those who
    satisfy a lot of payloads go down first.
 3. Media number, to minimize disk swapping.
 4. By AdobeCode just to make sure the order has no wiggle room.

*/
function PayloadSortOperationOrder(inInstallerPayloadA, inInstallerPayloadB)
{
	var a = inInstallerPayloadA;
	var b = inInstallerPayloadB;

	// If nobody depends on either a or b, favor hidden payloads
	// In principle this should only ever play a role when comparing the driver and a visible
	// payload with no dependents.  Since a visible payload can never be a dependent of
	// the driver, this non-dependency based primary ordering should never violate dependencies.

	/*
	// This causes obvious sort instability with extendscript and subtle instability with WebKit and/or IE
	if (a.policyNode && a.policyNode.isSessionDriver && b.policyNode && b.policyNode.visible)
	{
		return -1;
	}
	if (b.policyNode && b.policyNode.isSessionDriver && a.policyNode && a.policyNode.visible)
	{
		return 1;
	}
	*/

	// Low dependencies first
	var incomingDelta = a.GetRequiredArray().length - b.GetRequiredArray().length;
	if (incomingDelta != 0)
	{
		return incomingDelta;
	}

	// High dependents first
	var outgoingDelta = b.GetSatisfiedArray().length - a.GetSatisfiedArray().length;
	if (outgoingDelta != 0)
	{
		return outgoingDelta;
	}
	
	// Low media first
	var mediaWeight1 = 0;
	var mediaWeight2 = 0;
	if ((a.GetSessionData()).MediaInfo && (a.GetSessionData()).MediaInfo.volumeIndex)
		mediaWeight1 = (a.GetSessionData()).MediaInfo.volumeIndex;
	if ((b.GetSessionData()).MediaInfo && (b.GetSessionData()).MediaInfo.volumeIndex)
		mediaWeight2 = (b.GetSessionData()).MediaInfo.volumeIndex;
	var mediaDelta = mediaWeight1 - mediaWeight2;
	if (mediaDelta != 0)
	{
		return mediaDelta;
	}
		
	// Names
	if (a.GetProductName() < b.GetProductName())
	{
		return -1;
	}
	if (a.GetProductName() > b.GetProductName())
	{
		return 1;
	}

	// Low AdobeCodes first
	if (a.GetAdobeCode() == b.GetAdobeCode())
		return 0;
	return a.GetAdobeCode() < b.GetAdobeCode() ? -1 : 1;
}


/**
Sort InstallerPayload objects in dependency order for operations.

@param inPayloads an array or map (object) of InstallerPayload objects.
@param inOptReverse optional parameter, if true the sort order is reversed.
*/
function PayloadDependencySort(inPayloads, inOptReverse)
{
	// Transport to an array
	var result = new Array();
	for (p in inPayloads)
		result.push(inPayloads[p]);

	// Sort-o-matic
	result.sort(PayloadSortOperationOrder);
	
	// Reverse-o-matic?
	if (inOptReverse)
		result.reverse();

	//LogPayloadSet(gSession, "PayloadDependencySort result", result);

	return result;
}


/**
Sort InstallerPayload objects in UI order.

@param inPayloads an array or map (object) of InstallerPayload objects.
@param inSession a session object to determine if a payload is a driver.
*/
function PayloadUISort(inPayloads, inSession)
{
	// Transport to an array
	var result = new Array();
	for (p in inPayloads)
		result.push(inPayloads[p]);

	var sortpredicate = function(a, b)
	{
		if (inSession)
		{
			if (a.IsDriverForSession(inSession))
				return -1;
			if (b.IsDriverForSession(inSession))
				return 1;
		}
		if (a.GetProductName() == b.GetProductName())
		{
			return 0;
		}
		return a.GetProductName() < b.GetProductName() ? -1 : 1;
	};

	// Sort-o-matic
	result.sort(sortpredicate);
	
	return result;
}


/**
Convenience function to accumulate a map or array of InstallerPayloads into a map.
*/
function AccumulatePayloads(ioMap, inPayloads)
{
	for (var p in inPayloads)
	{
		ioMap[inPayloads[p].GetAdobeCode()] = inPayloads[p];
	}
	return ioMap;
}


/**
Dumb payload set intersection.
*/
function PayloadIntersect(inSet1, inSet2)
{
	var result = new Array();
	for (p1 in inSet1)
	{
		for (p2 in inSet2)
		{
			if (inSet1[p1].GetAdobeCode() == inSet2[p2].GetAdobeCode())
			{
				result.push(inSet1[p1]);
			}
		}
	}
	return result;
}


/**
Dump a set of payloads to log.

@param inSession a session object for access to the LogDebug method.
@param inHeader text written to the log before and after the payload list
@param inPayloads a collection (list or map) of InstallerPayload objects
@param inOptAppend static text or a function to append to each payload logged.
If a function is supplied, the function is passed the InstallerPayload object.
*/
function LogPayloadSet(inSession, inHeader, inPayloads, inOptAppend)
{
	var messages = new Array();
	for (var p in inPayloads)
	{
		var text = "  " + inPayloads[p].LogID();
		if (inOptAppend)
		{
			text += ": ";

			if ("function" == typeof inOptAppend)
			{
				text += inOptAppend(inPayloads[p]);
			}
			else
			{
				text += inOptAppend;
			}
		}
		messages.push(text);
	}
	if (messages.length > 0)
	{
		inSession.LogDebug("BEGIN " + inHeader);
		for (m in messages)
		{
			inSession.LogDebug(messages[m]);
		}
		inSession.LogDebug("END " + inHeader);
	}
	else
	{
		inSession.LogDebug("EMPTY SET " + inHeader);
	}
}


/**
Clone an object in a way that is not safe for object graphs with cycles.
*/
function CloneAcyclicObject(inSource, inTarget)
{
	for (var eachProp in inSource)
	{
		if (null == inSource[eachProp])
		{
			inTarget[eachProp] = null;
		}
		else if (typeof(inSource[eachProp]) == 'object')
		{
			inTarget[eachProp] = new Object();
			CloneAcyclicObject(inSource[eachProp], inTarget[eachProp]);
		}
		else
		{
			inTarget[eachProp] = inSource[eachProp].valueOf();
		}					
	}		
}


/**
Determine if an object has enumerable properties.
*/
var ContainerNotEmpty = function(inObj)
{
	if (inObj)
	{
		for (var p in inObj)
		{
			return true;
		}
	}
	return false;
};


/**
Given a collection of InstallerPayload objects, return an array
of session names (driver.GetProductName()) associated with the payload(s).
if inOptSession is provided, the driver of that session is excluded
from the list.  Useful for the common case where you want to list
OTHER sessions in which a payload is installed.
*/
function SessionNamesFromPayloads(inPayloadList, inOptSession)
{
	var sessionMap = new Object();
	var addAdobeSetup = false;

	// For each payload
	for (var oldAdobeCode in inPayloadList)
	{
		// find the collections
		var collectionList = inPayloadList[oldAdobeCode].GetCollectionRecords();
		for (var oldCollection in collectionList)
		{
			// Grab the driver's name
			var collectionDriver = collectionList[oldCollection].collection.driverPayload;
			if (collectionDriver)
			{
				if (inOptSession)
				{
					if (!collectionDriver.IsDriverForSession(inOptSession))
					{
						sessionMap[collectionDriver.GetAdobeCode()] = collectionDriver.GetProductName();
					}
				}
				else
				{
					sessionMap[collectionDriver.GetAdobeCode()] = collectionDriver.GetProductName();
				}
			}

			// Or flag to add "Adobe Setup" for collections sans-driver
			else
			{
				addAdobeSetup = true;
			}
		}
	}

	// Assemble an ordered array
	var sessionList = new Array();
	for (var adobeCode in sessionMap)
		sessionList.push(sessionMap[adobeCode]);
	if (addAdobeSetup)
		sessionList.push("Adobe Setup");
	sessionList.sort();

	return sessionList;
};

/*************************************************************************
ColoredLine and System Requirements Checks
**************************************************************************/

// Constructs an InclusiveRange instance.
// null can be used as an endpoint, in which
// case the range is considered to extend to
// infinity in the direction of that endpoint.
// An exception will be thrown if inStart comes after inEnd.
function InclusiveRange(inStart, inEnd)
{
	if ((null != inStart) && (null != inEnd) && (Number(inStart) > Number(inEnd)))
	{
		throw "InclusiveRange cannot be created with endpoints " + Number(inStart) + "and " + Number(inEnd);
	}
	
	this.start = (null != inStart) ? Number(inStart) : null;
	this.end = (null != inEnd) ? Number(inEnd) : null;
}


// Returns the maximum of two range end values, with null being the maximum end
InclusiveRange.prototype.GetMaxEnd = function(inRange)
{
	var result = null;
	
	if ((null != this.end) && (null != inRange.end))
	{
		result = (this.end > inRange.end) ? this.end : inRange.end;
	}
	else
	{
		result = null;
	}
	
	return result;
}


// Returns the minimum of two range end values, with null being the maximum end
InclusiveRange.prototype.GetMinEnd = function(inRange)
{
	var result = null;
	
	if (null != this.end)
	{
		if (null != inRange.end)
		{
			result = (this.end < inRange.end) ? this.end : inRange.end;
		}
		else
		{
			result = this.end;
		}
	}
	else
	{
		result = inRange.end;
	}
	
	return result;
}


// Returns the maximum of two range start values, with null being the minimal start
InclusiveRange.prototype.GetMaxStart = function(inRange)
{
	var result = null;
	
	if (null != this.start)
	{
		if (null != inRange.start)
		{
			result = (this.start > inRange.start) ? this.start : inRange.start;
		}
		else
		{
			result = this.start;
		}
	}
	else
	{
		result = inRange.start;
	}
	
	return result;
}


// Returns the minimum of two range start values, with null being the minimal start
InclusiveRange.prototype.GetMinStart = function(inRange)
{
	var result = null;
	
	if ((null != this.start) && (null != inRange.start))
	{
		result = (this.start < inRange.start) ? this.start : inRange.start;
	}
	else
	{
		result = null;
	}
	
	return result;
}


// returns the relationship between this' start and inRange start.
// negative === this.start comes 'before' inRange.start
// zero === this.start is equivalent to inRange.start
// positive === this.start comes 'after' inRange.start
InclusiveRange.prototype.CompareStart = function(inRange)
{
	var result = 0;
	
	if (null != this.start)
	{
		if (null != inRange.start)
		{
			if (this.start < inRange.start)
			{
				// lhs value < rhs value
				result = -1;
			}
			else if (this.start == inRange.start)
			{
				// lhs value == rhs value
				result = 0;
			}
			else
			{
				// lhs value > rhs value
				result = 1;
			}
		}
		else
		{
			// lhs value > null
			result = 1;
		}
	}
	else
	{
		if (null != inRange.start)
		{
			// null < rhs value
			result = -1;
		}
		else
		{
			// null == null
			result = 0;
		}
	}
	
	return result;
}


// returns the relationship between this' end and inRange end.
// negative === this.end comes 'before' inRange.end
// zero === this.end is equivalent to inRange.end
// positive === this.end comes 'after' inRange.end
InclusiveRange.prototype.CompareEnd = function(inRange)
{
	var result = 0;
	
	if (null != this.end)
	{
		if (null != inRange.end)
		{
			if (this.end < inRange.end)
			{
				// lhs value < rhs value
				result = -1;
			}
			else if (this.end == inRange.end)
			{
				// lhs value == rhs value
				result = 0;
			}
			else
			{
				// lhs value > rhs value
				result = 1;
			}
		}
		else
		{
			// lhs value < null
			result = -1;
		}
	}
	else
	{
		if (null != inRange.end)
		{
			// null > rhs value
			result = 1;
		}
		else
		{
			// null == null
			result = 0;
		}
	}
	
	return result;
}


// Returns a range that represents the intersection of this and the
// specified range, or null if there is no intersection
InclusiveRange.prototype.GetIntersection = function(inRange)
{
	var result = null;
	
	// intersectionStart = max(this.start, inRange.start)
	// null if start is beginning of line
	var intersectionStart = this.GetMaxStart(inRange);
	
	// intersectionEnd = min(this.end, inRange.end)
	// null if end is end of line
	var intersectionEnd = this.GetMinEnd(inRange);
	
	// result is only defined if [start, end] defines a range
	// null in either is considered that endpoint's infinity
	if ((null != intersectionStart) && (null != intersectionEnd))
	{
		if (intersectionStart <= intersectionEnd)
		{
			result = new InclusiveRange(intersectionStart, intersectionEnd);
		}
		else
		{
			result = null;
		}
	}
	else
	{
		result = new InclusiveRange(intersectionStart, intersectionEnd);
	}
	
	return result;
}


// Returns an array of two ranges that represent the inverse of this range
// The first of these is the range that comes before this one
// The second of these is the range that comes after this one
// If there is no before and/or after range, that element in the result
// array will be set to null.  The result of this function is always
// an array of two elements, though those elements may be null.
InclusiveRange.prototype.GetInverse = function()
{
	var result = new Array();
	
	var beforeRange = null;
	if (null != this.start)
	{
		beforeRange = new InclusiveRange(null, this.start - 1);
	}
	result.push(beforeRange);
	
	var afterRange;
	if (null != this.end)
	{
		afterRange = new InclusiveRange(this.end + 1, null);
	}
	result.push(afterRange);
	
	return result;
}


// Returns an array of one or more ranges that represent the union
// of this and the specified range.  The result will always be
// an array and will always contain at least one range.  The ranges
// will be non-overlapping, non-abutting, and will be in sorted
// order such that ranges that are numerically lower on a number line
// appear earlier in the array than ranges that come later.
InclusiveRange.prototype.GetUnion = function(inRange)
{
	var result = new Array();
	
	// If the ranges overlap or abut, the union is one range
	// with their maximal start and end points.
	// If the ranges do not overlap, the union is the two
	// ranges in sorted order.
	var intersection = this.GetIntersection(inRange);
	if (null == intersection)
	{
		// THE RANGES DO NOT OVERLAP, but may abut
		// order them
		var firstRange = null;
		var secondRange = null;
		var minStart = this.GetMinStart(inRange);
		if (minStart == this.start)
		{
			firstRange = this;
			secondRange = inRange;
		}
		else
		{
			firstRange = inRange;
			secondRange = this;
		}
		
		// check for abutting ranges
		if ((firstRange.end + 1) == secondRange.start)
		{
			// the ranges abut, merge them
			result.push(new InclusiveRange(firstRange.start, secondRange.end));
		}
		else
		{
			// the ranges do not abut, result is the two of them in order
			result.push(firstRange);
			result.push(secondRange);
		}
	}
	else
	{
		// THE RANGES OVERLAP
		// result is [minimum start, maximum end]
		var resultStart = this.GetMinStart(inRange);
		var resultEnd = this.GetMaxEnd(inRange);
		result.push(new InclusiveRange(resultStart, resultEnd));
	}
	
	return result;
}


function ColoredLine()
{
	var fullColorlessRange = new InclusiveRange(null, null);
	fullColorlessRange.color = null;
	
	this.coloredRanges = new Array();
	this.coloredRanges.push(fullColorlessRange);
}


// Internal method that replaces adjascent ranges on the colored line
// that have the same color with a single range that represents the
// entire span.
ColoredLine.prototype.MergeAdjascentSameColorRanges = function()
{
	var newColoredRanges = new Array();
	var accumulatedRange = null;
	
	for (var i = 0; i < this.coloredRanges.length; ++i)
	{
		var curRange = this.coloredRanges[i];
		
		if (accumulatedRange)
		{
			if (curRange.color == accumulatedRange.color)
			{
				accumulatedRange.end = curRange.end;
			}
			else
			{
				newColoredRanges.push(accumulatedRange);
				accumulatedRange = curRange;
			}
		}
		else
		{
			accumulatedRange = curRange;
		}
	}
	
	if (accumulatedRange)
	{
		newColoredRanges.push(accumulatedRange);
	}
	
	this.coloredRanges = newColoredRanges;
}


// Paints "holes" in the specified range with the specified color.
// Typically "holes" are those sub-ranges for which a color is not
// already specified, however this default behavior can be overridden
// by specifying an alternate key color.  When a gap key color has been
// specified, only the portion of the range already in that color is painted.
ColoredLine.prototype.PaintGapsInInclusiveRange = function(inRange, inColor, inGapKeyColor)
{
	this.PaintInclusiveRange(inRange,
								  inColor,
								  function(inOriginalColor, inReplacementColor)
								  {
									  var result = inOriginalColor;
									  if (inOriginalColor == inGapKeyColor)
									  {
										  result = inReplacementColor;
									  }
									  return result;
								  }
								 );
}


// Paint the specified range of the line with the specified color.
// If a color function is specified, it is called with the original color
// and the painting color and is responsible for returning the color to actually use.
// If no color function is specified, the paint color is used and overwrites the
// original color completely.
ColoredLine.prototype.PaintInclusiveRange = function(inRange, inColor, inColorFunction)
{
	var newColoredRanges = new Array();
	
	// replace each range with up to three components that exaclty fill that range
	// where the first is the portion of the range prior to the intersection
	// with the painted range, the middle is the portion of the range that
	// represents the intersection, and the last is the portion of the range
	// after the intersection
	for (var i = 0; i < this.coloredRanges.length; ++i)
	{
		var curRange = this.coloredRanges[i];
		var tempRange = null;
		var intersection = curRange.GetIntersection(inRange);
		if (intersection)
		{
			// if there is a segment of curRange before the intersection,
			// keep that segment with its existing color
			if (curRange.CompareStart(intersection) < 0)
			{
				tempRange = new InclusiveRange(curRange.start, intersection.start - 1);
				tempRange.color = curRange.color;
				newColoredRanges.push(tempRange);
			}
			
			// output the intersection in the new color or the one returned
			// by the color function if one has been provided
			if (null != inColorFunction)
			{
				intersection.color = inColorFunction(curRange.color, inColor);
			}
			else
			{
				intersection.color = inColor;
			}
			newColoredRanges.push(intersection);
			
			// if there is a segment of curRange after the intersection,
			// ouput that in the original color
			if (curRange.CompareEnd(intersection) > 0)
			{
				tempRange = new InclusiveRange(intersection.end + 1, curRange.end);
				tempRange.color = curRange.color;
				newColoredRanges.push(tempRange);
			}
		}
		else
		{
			newColoredRanges.push(curRange);
		}
	}
	
	this.coloredRanges = newColoredRanges;
	this.MergeAdjascentSameColorRanges();
}


// Overpaints this line with another one using the specified painting function.
// See PaintInclusiveRange() for details on inColorFunction details.
ColoredLine.prototype.PaintWithLine = function(inPaintLine, inColorFunction)
{
	for (var i = 0; i < inPaintLine.coloredRanges.length; ++i)
	{
		var curPaintRange = inPaintLine.coloredRanges[i];
		
		this.PaintInclusiveRange(curPaintRange, curPaintRange.color, inColorFunction);
	}
}


// Returns the color of the specified point on the line
ColoredLine.prototype.GetPointColor = function(inPointValue)
{
	var resultColor = null;
	
	if (null == inPointValue)
		throw "Invalid null parameter in ColoredLine.GetPaintColor().";
	
	var pointRange = new InclusiveRange(Number(inPointValue), Number(inPointValue));
	for (var i = 0; i < this.coloredRanges.length; ++i)
	{
		var curRange = this.coloredRanges[i];
		if (null != curRange.GetIntersection(pointRange))
		{
			resultColor = curRange.color;
			break;
		}
	}
	
	return resultColor;
}


// Returns whether or not the colored line includes the specified
// inColor (or has a colorless patch if inColor is null)
ColoredLine.prototype.ContainsColor = function(inColor)
{
	var result = false;
	
	for (var i = 0; i < this.coloredRanges.length; ++i)
	{
		if (inColor == this.coloredRanges[i].color)
		{
			result = true;
			break;
		}
	}
	
	return result;
}


// Returns an ordered array of ranges.  Those that have a color have
// a 'color' attribute whose value is the color.  The resulting range
// array represents an entire number line, with each range abutting
// the next.  The ranges are all inclusive ranges, with the endpoints
// having a null value.
ColoredLine.prototype.GetColoredRanges = function()
{
	var result = new Array(this.coloredRanges.length);
	
	for (var i = 0; i < this.coloredRanges.length; ++i)
	{
		var curRange = this.coloredRanges[i];
		var coloredRangeClone = new InclusiveRange(curRange.start, curRange.end);
		coloredRangeClone.color = curRange.color;
		result[i] = coloredRangeClone;
	}
	
	return result;
}


function RunSystemRequirementsCheck(inSession, inOptPayloadList)
{
	var requirementsNotMetErrors;
	var requirementsNotMetWarnings;
	var excludedSystemErrors;
		
	// Setup the payloads to look at, defaulting to the whole session.
	var payloadsToCheck = inOptPayloadList;
	if (null == payloadsToCheck)
	{
		payloadsToCheck = inSession.sessionPayloads;
	}

	try
	{	
		var systemInfo = inSession.GetSystemInfo();
		var systemRequireSet = null;

		var currentMemory = parseInt(systemInfo.Memory.system);		
		var currentDisplayWidth = parseInt(systemInfo.Display.width);
		var currentDisplayHeight = parseInt(systemInfo.Display.height);
		var currentMacOSMajor = 0;
		var currentMacOSMinor = 0;
		var currentMacOSRevision = 0;
		var currentMacOS64Bit = false;
		var currentMacOSServer = false;
		
		var currentPlatform = systemInfo.Macintosh ? "OSX" : "Win32";
		var sysReqTypes = new Array("Default", "OSX", "Win32");
		
		var errorCount = 0;
		var warningCount = 0;
		
		var SystemMemoryWarning = null;
		var SystemMemoryError = null;
		var minSysMemory = 0;
		var excludeSysMemory = 0;	
		var tempMemoryMin = 0;
		var tempMemoryExclude = 0;	
		
		var networkProtocolPorts = new Object;
		
		var DisplayWarning = null;
		var DisplayError = null;		
		var minDisplayWidth = 0;
		var excludeDisplayWidth = 0;
		var minDisplayHeight = 0;
		var excludeDisplayHeight = 0;	

		var CPUWarningMMX = null;		
		var CPUErrorMMX = null;	
		var CPUWarningNX = null;		
		var CPUErrorNX = null;
		var CPUWarningPAE = null;		
		var CPUErrorPAE = null;	
		var CPUWarningPAE = null;		
		var CPUErrorSSE = null;
		var CPUWarningSSE = null;		
		var CPUErrorSSE2 = null;
		var CPUWarningSSE2 = null;		
		var CPUError3DNow = null;
		var CPUWarning3DNow = null;		
		var CPUErrorAltivec = null;	
		var CPUWarningAltivec = null;				
		
		var currentWinOSMajor = 0;
		var currentWinOSMinor = 0;
		var currentWinSPMajor = 0;
		var currentWinSPMinor = 0;
		var currentWinOS64Bit = 0;
		var currentWinOSVariantName = null;
		var WinOSAccumulatedRequirements = new Object();
		var MacHasAnyOSRequirementData = false;
		var MacSupportedOSVersionMin = null;
		var MacSupportedOSPointSamples = null;
		var MacExcludedOSVersionUpperBound = null;
		var MacExcludedOSPointSamples = null;
		var validOSXP = true;
		var validOSVista = true;
		var validOSServer2003 = true;
		
		// TODO collect space information for requirements
		// TODO collect space information for the startup disk		
		var neededStartupSpace = 100;
		var haveStartupSpace = 1000;
		var StartupSpaceError = null;
		
		var errorStringMap = new Object();							

		if ("OSX" == currentPlatform)
		{
			currentMacOSMajor = parseInt(systemInfo.Macintosh.VersionMajor);
			currentMacOSMinor = parseInt(systemInfo.Macintosh.VersionMinor);
			currentMacOSRevision = parseInt(systemInfo.Macintosh.VersionDot);
			currentMacOSVersion = String(currentMacOSMajor) + "." + String(currentMacOSMinor) + "." + String(currentMacOSRevision);
			currentMacOS64Bit = systemInfo.Macintosh.Is64Bit ? true : false;
			currentMacOSServer = systemInfo.Macintosh.IsServer ? true : false;	
			
			inSession.properties["OS64Bit"] = systemInfo.Macintosh.Is64Bit ? 1 : 0;			
		}
		else if ("Win32" == currentPlatform)
		{
			currentWinOSMajor = parseInt(systemInfo.Windows.VersionMajor);
			currentWinOSMinor = parseInt(systemInfo.Windows.VersionMinor);
			currentWinSPMajor = parseInt(systemInfo.Windows.SPMajor);
			currentWinSPMinor = parseInt(systemInfo.Windows.SPMinor);						
			currentWinOS64Bit = ("1" == systemInfo.Windows.Is64Bit) ? true : false;
			var windowsType = parseInt(systemInfo.Windows.ProductType);
			
			if (!((currentWinOSMajor == 6) && (currentWinOSMinor == 0)))
				validOSVista = false;

			if (!(5 == currentWinOSMajor && 1 == currentWinOSMinor))
				validOSXP = false;
			
			if ((5 == currentWinOSMajor) &&  (2 == currentWinOSMinor))
			{
				if (1 == windowsType)
				{
					validOSServer2003 = false;
					validOSXP = true;
				}
				else
				{
				 	validOSXP = false;	
					validOSServer2003 = true;				 			
				}	
			}
						
			if (validOSXP)
				currentWinOSVariantName = "XP";
			else if (validOSVista)
				currentWinOSVariantName = "Vista";
			else if (validOSServer2003)
				currentWinOSVariantName = "Server2003";
			else
				currentWinOSVariantName = null;
			
			inSession.properties["OS64Bit"] = ("1" == systemInfo.Windows.Is64Bit) ? 1 : 0;
		}	
		
		// Startup space checks
		if (neededStartupSpace > haveStartupSpace)
		{
			errorStringMap["neededStartupSpace"] = bytesToText(neededStartupSpace * 1048576);
			StartupSpaceError = new LocalizedString("systemPageHardDiskSpaceTarget", null, errorStringMap);
		
			errorCount++;
		}

		// For each payload...
		for (p in payloadsToCheck)
		{
			// If it has system requirements...
			systemRequireSet = _jsonToObject(payloadsToCheck[p].GetSessionData().SystemRequirements);
			
			if (systemRequireSet && systemRequireSet[0])
			{
				for (var i = 0; i < systemRequireSet.length; i++)
				{
					// System Memory =========================================================
					if (systemRequireSet[i].Memory)
					{
						if (systemRequireSet[i].Memory.System)
						{
							var accessPoint = null;
							
							// For each of Default, OSX, and Win32
							for (var k = 0; k < sysReqTypes.length; k++) 
							{
								
								if ("OSX" == sysReqTypes[k])
									accessPoint = systemRequireSet[i].Memory.System.OSX;
								else if ("Win32" == sysReqTypes[k])
									accessPoint = systemRequireSet[i].Memory.System.Win32;
								else
									accessPoint = systemRequireSet[i].Memory.System.Default;
								
								// Only check Default and the current platform
								if (accessPoint && ((sysReqTypes[k] == "Default") || (sysReqTypes[k] == currentPlatform)))
								{
									tempMemoryMin = 0;
									tempMemoryExclude = 0;
									
									if (accessPoint.Require)
										tempMemoryMin = parseInt(accessPoint.Require);
									
									if (accessPoint.Exclude)
										tempMemoryExclude = parseInt(accessPoint.Exclude);
									
									// Only update min and exclude if they are greater than the previous runs
									if (tempMemoryMin > minSysMemory)
										minSysMemory = tempMemoryMin;
									
									if (tempMemoryExclude > excludeSysMemory)
										excludeSysMemory = tempMemoryExclude;
									
									if (currentMemory <= excludeSysMemory)
									{
										if (currentMemory < minSysMemory)
										{
											errorStringMap["minMemory"] = bytesToText(minSysMemory * 1048576);
											SystemMemoryError = new LocalizedString("systemPageMemoryRequire", null, errorStringMap);
										}
										else
										{
											errorStringMap["excludeMemory"] = bytesToText(excludeSysMemory * 1048576);
											SystemMemoryError = new LocalizedString("systemPageMemoryExclude", null, errorStringMap);
										}
										errorCount++;
									}
									else if (currentMemory < minSysMemory)
									{
										errorStringMap["minMemory"] = bytesToText(minSysMemory * 1048576);
										SystemMemoryWarning = new LocalizedString("systemPageMemoryRequire", null, errorStringMap);
										warningCount++;
									}
								}
							}
						}
					}
					
					// Networking ======================================================
					if (systemRequireSet[i].Networking)
					{
						// Accumulate network port entries into networkProtocolPorts
						var portEntry = systemRequireSet[i].Networking.Port;
						if (portEntry)
						{
							var portNumber = portEntry.number;
							if (portNumber)
							{
								var theProtocol = portEntry["protocol"];
								if (null == theProtocol)
								{
									theProtocol = "tcp";
								}
								
								var isOptional = Number(portEntry["optional"]);
								if (isOptional != 1)
								{
									if (null == networkProtocolPorts[theProtocol])
										networkProtocolPorts[theProtocol] = new Object;
								
									networkProtocolPorts[theProtocol][String(portNumber)] = "warn";							
								}
							}
							else
							{
								inSession.LogWarning("Networking system requirements port entry exists without a port number.");
							}
						}
						else
						{
							inSession.LogWarning("Networking system requirements entry exists without a port entry.");
						}
					}
					
					// Display =========================================================
					if (systemRequireSet[i].Display)
					{
						var accessPoint = null;
						
						// For each of Default, OSX, and Win32
						for (var k = 0; k < sysReqTypes.length; k++) 
						{
							
							if ("OSX" == sysReqTypes[k])
								accessPoint = systemRequireSet[i].Display.OSX;
							else if ("Win32" == sysReqTypes[k])
								accessPoint = systemRequireSet[i].Display.Win32;
							else
								accessPoint = systemRequireSet[i].Display.Default;
							
							// Only check Default and the current platform
							if (accessPoint && ((sysReqTypes[k] == "Default") || (sysReqTypes[k] == currentPlatform)))
							{						
								tempWidthMin = 0;
								tempWidthExclude = 0;
								tempHeightMin = 0;
								tempHeightExclude = 0;							
								
								if (accessPoint.Require && accessPoint.Require.Width)
									tempWidthMin = parseInt(accessPoint.Require.Width);
								
								if (accessPoint.Require && accessPoint.Require.Height)
									tempHeightMin = parseInt(accessPoint.Require.Height);
								
								if (accessPoint.Exclude && accessPoint.Exclude.Width)
									tempWidthExclude = parseInt(accessPoint.Exclude.Width);
								
								if (accessPoint.Exclude && accessPoint.Exclude.Height)
									tempHeightExclude = parseInt(accessPoint.Exclude.Height);
								
								// Only update min and exclude for width and height if they are greater than the previous run
								if (tempWidthMin > minDisplayWidth)
									minDisplayWidth = tempWidthMin;
								
								if (tempHeightMin > minDisplayHeight)
									minDisplayHeight = tempHeightMin;
								
								if (tempWidthExclude > excludeDisplayWidth)
									excludeDisplayWidth = tempWidthExclude;
								
								if (tempHeightExclude > excludeDisplayHeight)
									excludeDisplayHeight = tempHeightExclude;								
								
								if (currentDisplayWidth <= excludeDisplayWidth || currentDisplayHeight <= excludeDisplayHeight)
								{
									if (currentDisplayWidth < minDisplayWidth || currentDisplayHeight < minDisplayHeight)
									{
										errorStringMap["minMonitorResW"] = minDisplayWidth;
										errorStringMap["minMonitorResH"] = minDisplayHeight;
										DisplayError = new LocalizedString("systemPageMonitorRequire", null, errorStringMap);
									}
									else
									{
										errorStringMap["excludeMonitorResW"] = minDisplayWidth;
										errorStringMap["excludeMonitorResH"] = minDisplayHeight;
										DisplayError = new LocalizedString("systemPageMonitorExclude", null, errorStringMap);										
									}
									errorCount++;
								}
								else if (currentDisplayWidth < minDisplayWidth || currentDisplayHeight < minDisplayHeight)
								{
									errorStringMap["minMonitorResW"] = minDisplayWidth;
									errorStringMap["minMonitorResH"] = minDisplayHeight;
									DisplayWarning = new LocalizedString("systemPageMonitorRequire", null, errorStringMap);
									warningCount++;
								}
							}
						}
					}					

					// CPU =========================================================
					if (systemRequireSet[i].CPU)
					{
						var accessPoint = null;
						
						for (var k = 0; k < sysReqTypes.length; k++) 
						{

							if ("OSX" == sysReqTypes[k])
								accessPoint = systemRequireSet[i].CPU.OSX;
							else if ("Win32" == sysReqTypes[k])
								accessPoint = systemRequireSet[i].CPU.Win32;
							else
								accessPoint = systemRequireSet[i].CPU.Default;
				
							// Only check Default and the current platform
							if (accessPoint && ((sysReqTypes[k] == "Default") || (sysReqTypes[k] == currentPlatform)))
							{
								// Error for requirements not met 
								if (accessPoint.Require && systemInfo.CPU)
								{
									if (accessPoint.Require.MMX) {
										if (systemInfo.CPU.MMX) {
											if ("1" != systemInfo.CPU.MMX)
											{
												CPUErrorMMX = new LocalizedString("systemPageCPURequireMMX");
												errorCount++;
											}
										}
										else 
										{
											CPUErrorMMX = new LocalizedString("systemPageCPURequireMMX");
											errorCount++;
										}
									}
												
									if (accessPoint.Require.NX) {
										if (systemInfo.CPU.NX)
										{
											if ("1" != systemInfo.CPU.NX)
											{
												CPUErrorNX = new LocalizedString("systemPageCPURequireNX");
												errorCount++;
											}	
										}
										else 
										{
											CPUErrorNX = new LocalizedString("systemPageCPURequireNX");
											errorCount++;	
										}
									}
												
									if (accessPoint.Require.PAE) {
										if (systemInfo.CPU.PAE)
										{
											if ("1" != systemInfo.CPU.PAE)
											{
												CPUErrorPAE = new LocalizedString("systemPageCPURequirePAE");
												errorCount++;
											}	
										}
										else 
										{
											CPUErrorPAE = new LocalizedString("systemPageCPURequirePAE");
											errorCount++;	
										}										
									}												

									if (accessPoint.Require.SSE)
									{
										if (systemInfo.CPU.SSE)
										{
											if ("1" != systemInfo.CPU.SSE)
											{
												CPUErrorSSE = new LocalizedString("systemPageCPURequireSSE");
												errorCount++;
											}
										}
										else 
										{
											CPUErrorSSE = new LocalizedString("systemPageCPURequireSSE");
											errorCount++;	
										}
									}
												
									if (accessPoint.Require.SSE2) 
									{
										if (systemInfo.CPU.SSE2)
										{	
											if ("1" != systemInfo.CPU.SSE2)
											{
												CPUErrorSSE2 = new LocalizedString("systemPageCPURequireSSE2");
												errorCount++;
											}	
										}
										else 
										{
											CPUErrorSSE2 = new LocalizedString("systemPageCPURequireSSE2");
											errorCount++;	
										}
									}												

									if (accessPoint.Require.ThreeDNOW)
									{
										if (systemInfo.CPU.ThreeDNOW)
										{
											if ("1" != systemInfo.CPU.ThreeDNOW)
											{
												CPUError3DNow = new LocalizedString("systemPageCPURequire3DNow");
												errorCount++;
											}	
										}
										else 
										{
											CPUError3DNow = new LocalizedString("systemPageCPURequire3DNow");
											errorCount++;	
										}
									}											

									if (accessPoint.Require.AltiVec)
									{
										if (systemInfo.CPU.Altivec)
										{
											if ("1" != systemInfo.CPU.Altivec)
											{
												CPUErrorAltivec = new LocalizedString("systemPageCPURequireAltiVec");
												errorCount++;
											}
										} 
										else 
										{
											CPUErrorAltivec = new LocalizedString("systemPageCPURequireAltiVec");
											errorCount++;	
										}	
									}	
								}

								// Error for exclusions encountered
								if (accessPoint.Exclude)
								{
									if (accessPoint.Exclude.MMX) {
										if (systemInfo.CPU.MMX) {
											CPUErrorMMX = new LocalizedString("systemPageCPUExcludeMMX");
											errorCount++;
										}
									}

									if (accessPoint.Exclude.NX) {
										if (systemInfo.CPU.NX)
										{
											CPUErrorNX = new LocalizedString("systemPageCPUExcludeNX");
											errorCount++;	
										}
									}

									if (accessPoint.Exclude.PAE) {
										if (systemInfo.CPU.PAE)
										{
											CPUErrorPAE = new LocalizedString("systemPageCPUExcludePAE");
											errorCount++;	
										}										
									}												

									if (accessPoint.Exclude.SSE)
									{
										if (systemInfo.CPU.SSE)
										{
											CPUErrorPAE = new LocalizedString("systemPageCPUExcludeSSE");
											errorCount++;	
										}
									}

									if (accessPoint.Exclude.SSE2) 
									{
										if (systemInfo.CPU.SSE2)
										{	
												CPUErrorSSE2 = new LocalizedString("systemPageCPUExcludeSSE2");
												errorCount++;	
										}
									}												

									if (accessPoint.Exclude.ThreeDNOW)
									{
										if (systemInfo.CPU.ThreeDNOW)
										{
											CPUError3DNow = new LocalizedString("systemPageCPUExclude3DNow");
											errorCount++;
										}
									}											

									if (accessPoint.Exclude.AltiVec)
									{
										if (systemInfo.CPU.Altivec)
										{
											CPUErrorAltivec = new LocalizedString("systemPageCPUExcludeAltiVec");
											errorCount++;
										} 
									}
								}
	
							}
						}
					}  

					// Operating System =========================================================
					if (systemRequireSet[i].OS)
					{
						var accessPoint = null;
						
						// Mac OS =========================================================	
						if (systemRequireSet[i].OS.Macintosh && ("OSX" == currentPlatform))
						{
							accessPoint = systemRequireSet[i].OS.Macintosh;
							MacHasAnyOSRequirementData = true;
							
							// Determine supported OS for current payload and
							// intersect with the overall supported OS values
							if (accessPoint.Require)
							{
								var payloadMacSupportedOSVersionMin = null;
								var payloadMacSupportedOSPointSamples = null;
								var payloadMacSupportedOSRequires64Bit = false;
								var payloadMacSupportedOSRequiresServer = false;
								
								// accumulate required version & point samples from payload
								for (var requireIndex = 0; requireIndex < accessPoint.Require.length; ++requireIndex)
								{
									var curRequire = accessPoint.Require[requireIndex];
									if ((null == curRequire["@lowerBound"]) || ("1" == curRequire["@lowerBound"]))
									{
										// require is a range, merge it with the existing payload range
										if (null == payloadMacSupportedOSVersionMin)
										{
											payloadMacSupportedOSVersionMin = curRequire;
										}
										else
										{
											payloadMacSupportedOSVersionMin = MaximizeRequiredRanges(payloadMacSupportedOSVersionMin, curRequire);
										}
									}
									else
									{
										// require is a point sample, add it to the set of payload point samples
										if (null == payloadMacSupportedOSPointSamples)
											payloadMacSupportedOSPointSamples = new Object();
										
										UnionRequiredPointSampleWithSet(payloadMacSupportedOSPointSamples, curRequire);
									}
								}
								
								// intersect payload required data with installer set
								if (payloadMacSupportedOSVersionMin)
								{
									if (MacSupportedOSVersionMin)
										MacSupportedOSVersionMin = MaximizeRequiredRanges(MacSupportedOSVersionMin, payloadMacSupportedOSVersionMin);
									else
										MacSupportedOSVersionMin = payloadMacSupportedOSVersionMin;
								}
								if (payloadMacSupportedOSPointSamples)
								{
									if (MacSupportedOSPointSamples)
										MacSupportedOSPointSamples = IntersectPointSampleSets(MacSupportedOSPointSamples, payloadMacSupportedOSPointSamples);
									else
										MacSupportedOSPointSamples = payloadMacSupportedOSPointSamples;
								}
							}
							
							if (accessPoint.Exclude)
							{
								var payloadMacExcludedOSVersionUpperBound = null;
								var payloadMacExcludedOSPointSamples = null;
								
								// accumulate exclude version & point samples from payload
								for (var excludeIndex = 0; excludeIndex < accessPoint.Exclude.length; ++excludeIndex)
								{
									var curExclude = accessPoint.Exclude[excludeIndex];
									
									// only check exclude if the OS attributes indicate it's appropriate to "include"
									if ((currentMacOS64Bit || ("1" != curExclude.Only64Bit))
										&& (currentMacOSServer || ("1" != curExclude.OnlyServer)))
									{
										if ((null == curExclude["@upperBound"]) || ("1" == curExclude["@upperBound"]))
										{
											// exclude is a range, merge it with the existing payload exclude range
											if (null == payloadMacExcludedOSVersionUpperBound)
											{
												payloadMacExcludedOSVersionUpperBound = curExclude.Version;
											}
											else
											{
												// "highest" exclude version takes precedence
												if (1 == compareVersions(curExclude.Version, payloadMacExcludedOSVersionUpperBound))
													payloadMacExcludedOSVersionUpperBound = curExclude.Version;
											}
										}
										else
										{
											// exclude is a point sample, add it to the set of payload exclude point samples
											if (null == payloadMacExcludedOSPointSamples)
												payloadMacExcludedOSPointSamples = new Object();
											
											payloadMacExcludedOSPointSamples[curExclude.Version] = 1;
										}
									}
								}
								
								// intersect payload exclusion data with installer set
								if (payloadMacExcludedOSVersionUpperBound)
								{
									if (MacExcludedOSVersionUpperBound)
									{
										if (1 == compareVersions(payloadMacExcludedOSVersionUpperBound, MacExcludedOSVersionUpperBound))
											MacExcludedOSVersionUpperBound = payloadMacExcludedOSVersionUpperBound;
									}
									else
									{
										MacExcludedOSVersionUpperBound = payloadMacExcludedOSVersionUpperBound;
									}
								}
								
								if (payloadMacExcludedOSPointSamples)
								{
									if (null == MacExcludedOSPointSamples)
										MacExcludedOSPointSamples = new Object();
									for (var aKey in payloadMacExcludedOSPointSamples)
									{
										MacExcludedOSPointSamples[aKey] = 1;
									}
								}
							}
						}
						
						// Win OS =========================================================
						if (systemRequireSet[i].OS.Windows && ("Win32" == currentPlatform))
						{
							// iterate through the Windows variant requirements
							for (var curVariantKey in systemRequireSet[i].OS.Windows)
							{								
								var curVariant = systemRequireSet[i].OS.Windows[curVariantKey];
								var curAccumulatedRequirements = new ColoredLine();
								
								// "paint" the payload's requirement line with all Exclusions
								if (curVariant.Exclude)
								{
									var curExclude = curVariant.Exclude;
									
									// Exclusions are only taken into account if the "Only" Exclude attributes
									// are met by the current OS
									if (currentWinOS64Bit || ("1" != curExclude.Only64Bit))
									{
										var maxSPValue = null;
										if (currentWinOS64Bit && curExclude["@servicePack64Bit"])
											maxSPValue = curExclude["@servicePack64Bit"];
										if (!maxSPValue && curExclude.MaxServicePack)
											maxSPValue = curExclude.MaxServicePack;
																																														
										if (maxSPValue)
										{
											// service pack specified, exclude that SP or up to and including that SP
											// based on @uppperBound
											if ((null != curExclude["@upperBound"])
												&& ("0" == curExclude["@upperBound"]))
											{
												// current Exclude is a point sample, paint a range of just that point
												curAccumulatedRequirements.PaintInclusiveRange(new InclusiveRange(maxSPValue, maxSPValue), "exclude");
											}
											else
											{
												// current Exclude is a range, paint the range up to and including that point
												curAccumulatedRequirements.PaintInclusiveRange(new InclusiveRange(null, maxSPValue), "exclude");
											}
										}
										else
										{
											// no service pack specified, exclude entire OS variant
											curAccumulatedRequirements.PaintInclusiveRange(new InclusiveRange(null, null), "exclude");
										}
									}
								}

								// "paint" the payload's requirement line with all Require
								// Require trumps exclude in a single payload
								if (curVariant.Require)
								{
									var curRequire = curVariant.Require;

									var minSPValue = null;
									if (currentWinOS64Bit && curRequire["@servicePack64Bit"])
										minSPValue = curRequire["@servicePack64Bit"];
									if (!minSPValue && curRequire.MinServicePack)
										minSPValue = curRequire.MinServicePack;
									
									if (minSPValue)
									{
										// if Require specifies a minimum service pack, include that SP or that SP and greater
										// based on @lowerBound
										if  ((null != curRequire["@lowerBound"])
											 && ("0" == curRequire["@lowerBound"]))
										{
											// current Require is a point sample, paint a range of just that point
											curAccumulatedRequirements.PaintInclusiveRange(new InclusiveRange(minSPValue, minSPValue), "require");
										}
										else
										{
											// current Require is a range, paint the range from that point to infinity
											curAccumulatedRequirements.PaintInclusiveRange(new InclusiveRange(minSPValue, null), "require");
										}
									}
									else
									{
										// no service pack specified, 'require' entire OS variant
										curAccumulatedRequirements.PaintInclusiveRange(new InclusiveRange(null, null), "require");
									}
									
									// 64-bit property is sticky
									if ("1" == curRequire.Need64Bit)
										curAccumulatedRequirements.need64Bit = true;
								}
								
								
								// Merge this payload's requirement with the combined accumulation
								if (null == WinOSAccumulatedRequirements[curVariantKey])
								{
									WinOSAccumulatedRequirements[curVariantKey] = curAccumulatedRequirements;
								}
								else
								{
									WinOSAccumulatedRequirements[curVariantKey].PaintWithLine(curAccumulatedRequirements,
											function(inOriginalColor, inPaintColor)
											{
												var colorToUse = inOriginalColor;
												
												// when merging requirements from payloads,
												// Exclude values union and trump Require
												// Require values intersect
												if ("exclude" == inPaintColor)
												{
													colorToUse = "exclude";
												}
												else if ("require" == inPaintColor)
												{
													if ("exclude" == inOriginalColor)
													{
														colorToUse = "exclude";
													}
													else if ("require" == inOriginalColor)
													{
														colorToUse = "require";
													}
													else
													{
														colorToUse = null;
													}
												}
												else
												{
													colorToUse = inOriginalColor;
												}
												
												return colorToUse;
											}
										);
									
									if (curAccumulatedRequirements.need64Bit)
										WinOSAccumulatedRequirements[curVariantKey].need64Bit = true;
								}
							}
						} // WinOS
					} // OS
				} // System requirement set [i]
			} // System requirements
		} // Payloads
		
		
		// Check whether or not the current Windows OS is supported
		var isSupportedWinOS = false;
		var isExcludedWinOS = false;
		if (WinOSAccumulatedRequirements)
		{
			if (null != currentWinOSVariantName)
			{
				var curVariantLine = WinOSAccumulatedRequirements[currentWinOSVariantName];
				if (curVariantLine)
				{
					var osColor = curVariantLine.GetPointColor(currentWinSPMajor);
					if (("exclude" == osColor) || (!currentWinOS64Bit && curVariantLine.need64Bit))
					{
						isSupportedWinOS = false;
						isExcludedWinOS = true;
					}
					else if ("require" == osColor)
					{
						isSupportedWinOS = true;
						isExcludedWinOS = false;
					}
					else
					{
						isSupportedWinOS = false;
						isExcludedWinOS = false;
					}
				}
			}
		}
		if (!isSupportedWinOS)
		{
			if (isExcludedWinOS)
				++errorCount;
			else
				++warningCount;
		}
		
		
		// determine whether or not there is an OS version problem on MacOS
		var isSupportedMacOS = !MacHasAnyOSRequirementData;
		var isExcludedMacOS = false;
		if (MacHasAnyOSRequirementData)
		{
			if (MacSupportedOSVersionMin)
			{
				// in supported range?
				isSupportedMacOS = ((compareVersions(currentMacOSVersion, MacSupportedOSVersionMin.Version) >= 0)
									&& (currentMacOS64Bit || (1 != MacSupportedOSVersionMin.Need64Bit))
									&& (currentMacOSServer || (1 != MacSupportedOSVersionMin.NeedServer)));
			}
			if (!isSupportedMacOS && MacSupportedOSPointSamples)
			{
				// in list of supported specific OSes?
				for (var aSupportedPointKey in MacSupportedOSPointSamples)
				{
					var aSupportedVersion = MacSupportedOSPointSamples[aSupportedPointKey];
					isSupportedMacOS = ((0 == compareVersions(currentMacOSVersion, aSupportedVersion.Version))
										&& (currentMacOS64Bit || (1 != aSupportedVersion.Need64Bit))
										&& (currentMacOSServer || (1 != aSupportedVersion.NeedServer)));
					if (isSupportedMacOS)
						break;
				}
			}
			if (MacExcludedOSVersionUpperBound)
			{
				// in excluded range?
				isExcludedMacOS = (compareVersions(currentMacOSVersion, MacExcludedOSVersionUpperBound) < 0);
			}
			if (!isExcludedMacOS && MacExcludedOSPointSamples)
			{
				// in list of excluded specific OSes?
				for (var anExcludedVersion in MacExcludedOSPointSamples)
				{
					isExcludedMacOS = (0 == compareVersions(currentMacOSVersion, anExcludedVersion));
					if (isExcludedMacOS)
						break;
				}
			}
		}
		if (!isSupportedMacOS)
		{
			if (isExcludedMacOS)
				++errorCount;
			else
				++warningCount;
		}
		
		
		// add network port errors if there are any
		var unavailableProtocolPorts = new Object;
		for (var aNetworkProtocolName in networkProtocolPorts)
		{
			// convert port dict to a numerically sorted array of port numbers to check
			var portArray = new Array();
			
			var aNetworkProtocolPortDict = networkProtocolPorts[aNetworkProtocolName];
			for (var aNetworkPort in aNetworkProtocolPortDict)
			{
				portArray.push(parseInt(aNetworkPort));
			}
			portArray.sort(function(a, b) { return a - b; });
			
			for (var portIndex = 0; portIndex < portArray.length; ++portIndex)
			{
				var portCheckResult = inSession.IsPortForProtocolAvailable(portArray[portIndex], aNetworkProtocolName);
				var portIsAvailable = (portCheckResult && portCheckResult.success);
				if (!portIsAvailable)
				{
					if (null == unavailableProtocolPorts[aNetworkProtocolName])
						unavailableProtocolPorts[aNetworkProtocolName] = new Array;
					
					unavailableProtocolPorts[aNetworkProtocolName].push(portArray[portIndex]);
					++warningCount;
				}
			}
		}
		
		// Display the system requirements errors
		if (errorCount > 0 || warningCount > 0) 
		{	
			requirementsNotMetErrors = new Array();
			requirementsNotMetWarnings = new Array();
			excludedSystemErrors = new Array();
			
			// Add the actual errors and the "correct" values for requirements
			
			// if there was a Win version problem, report it
			if (!isSupportedWinOS)
			{
				// if there are any require's, tell the user about the
				// required OS varieties, otherwise tell the user about
				// the excluded OS varieties
				var hasAnyRequire = false;
				for (var curVariantKey in WinOSAccumulatedRequirements)
				{
					var curVariantLine = WinOSAccumulatedRequirements[curVariantKey];
					
					if (curVariantLine.ContainsColor("require"))
					{
						hasAnyRequire = true;
						break;
					}
				}
				
				if (hasAnyRequire)
				{
					// if the OS is excluded, it's an error; if it's simply not supported, show warning
					var listToReportTo = isExcludedWinOS ? requirementsNotMetErrors : requirementsNotMetWarnings;
					
					// list required OS versions
					for (var curVariantKey in WinOSAccumulatedRequirements)
					{
						var curVariantLine = WinOSAccumulatedRequirements[curVariantKey];
						
						var curColoredRanges = curVariantLine.GetColoredRanges();
						for (var i = 0; i < curColoredRanges.length; ++i)
						{
							var curRange = curColoredRanges[i];
							
							if ("require" == curRange.color)
							{
								var formedOSDescription = DescribeWinOS(curVariantKey,
																			 curRange,
																			 curVariantLine.need64Bit);
								listToReportTo.push(formedOSDescription);
							}
						}
					}
				}
				else
				{
					// list excluded OS versions
					for (var curVariantKey in WinOSAccumulatedRequirements)
					{
						var curVariantLine = WinOSAccumulatedRequirements[curVariantKey];
						
						var curColoredRanges = curVariantLine.GetColoredRanges();
						for (var i = 0; i < curColoredRanges.length; ++i)
						{
							var curRange = curColoredRanges[i];
							
							if ("exclude" == curRange.color)
							{
								var formedOSDescription = DescribeWinOS(curVariantKey,
																			 curRange,
																			 curVariantLine.need64Bit);
								excludedSystemErrors.push(formedOSDescription);
							}
						}
					}
				}
			}
			
			
			// if there was a MacOS version problem, report it
			if (!isSupportedMacOS)
			{
				// post ranged requirement
				if (MacSupportedOSVersionMin)
				{
					var requirementString = RequireToString(MacSupportedOSVersionMin);
					if (isExcludedMacOS)
						requirementsNotMetErrors.push(requirementString);
					else
						requirementsNotMetWarnings.push(requirementString);
				}
				
				// post point requirements
				if (MacSupportedOSPointSamples)
				{
					for (var aSupportedPointKey in MacSupportedOSPointSamples)
					{
						var aSupportedVersion = MacSupportedOSPointSamples[aSupportedPointKey];
						var requirementString = RequireToString(aSupportedVersion);
						if (isExcludedMacOS)
							requirementsNotMetErrors.push(requirementString);
						else
							requirementsNotMetWarnings.push(requirementString);
					}
				}
			}
			
			// if there were any networking port problems, report them
			for (var aProtocolName in unavailableProtocolPorts)
			{
				var unavailablePorts = unavailableProtocolPorts[aProtocolName];
				for (var aPortIndex = 0; aPortIndex < unavailablePorts.length; ++aPortIndex)
				{
					var errorStringMap = new Object;
					errorStringMap["protocol"] = aProtocolName;
					errorStringMap["port"] = unavailablePorts[aPortIndex];
					requirementsNotMetWarnings.push(new LocalizedString("systemPageUnavailablePort", null, errorStringMap));
				}
			}
			
			if (errorCount > 0) {
				// Insert errors for all failures
				if (StartupSpaceError)
					requirementsNotMetErrors.push(StartupSpaceError);	
				if (SystemMemoryError)
					requirementsNotMetErrors.push(SystemMemoryError);
				if (DisplayError)
					requirementsNotMetErrors.push(DisplayError);
				if (CPUErrorMMX)
					requirementsNotMetErrors.push(CPUErrorMMX);
				if (CPUErrorNX)
					requirementsNotMetErrors.push(CPUErrorNX);
				if (CPUErrorPAE)
					requirementsNotMetErrors.push(CPUErrorPAE);
				if (CPUErrorSSE)
					requirementsNotMetErrors.push(CPUErrorSSE);
				if (CPUErrorSSE2)
					requirementsNotMetErrors.push(CPUErrorSSE2);			
				if (CPUError3DNow)
					requirementsNotMetErrors.push(CPUError3DNow);
				if (CPUErrorAltivec)
					requirementsNotMetErrors.push(CPUErrorAltivec);
			}
			
			if (warningCount > 0) 
			{
				// Insert warnings for all failures
				if (!SystemMemoryError && SystemMemoryWarning) 
					requirementsNotMetWarnings.push(SystemMemoryWarning);
				if (!DisplayError && DisplayWarning) 
					requirementsNotMetWarnings.push(DisplayWarning);
				if (!CPUErrorMMX && CPUWarningMMX) 
					requirementsNotMetWarnings.push(CPUWarningMMX);
				if (!CPUErrorNX && CPUWarningNX) 
					requirementsNotMetWarnings.push(CPUWarningNX);				
				if (!CPUErrorPAE && CPUWarningPAE) 
					requirementsNotMetWarnings.push(CPUWarningPAE);
				if (!CPUErrorSSE && CPUWarningSSE) 
					requirementsNotMetWarnings.push(CPUWarningSSE);
				if (!CPUErrorSSE2 && CPUWarningSSE2) 
					requirementsNotMetWarnings.push(CPUWarningSSE2);
				if (!CPUError3DNow && CPUWarning3DNow) 
					requirementsNotMetWarnings.push(CPUWarning3DNow);				
				if (!CPUErrorAltivec && CPUWarningAltivec) 
					requirementsNotMetWarnings.push(CPUWarningAltivec); 											
			}		
		}	
	}
	catch(e)
	{
		inSession.LogWarning("Exception while testing system requirements.");
		inSession.LogWarning(listProperties(e, "exception"));
	}	

	return new Array(requirementsNotMetErrors, requirementsNotMetWarnings, excludedSystemErrors);	
}

// maximizes all aspects of the required range
// version is set to the maximum, and the most
// restrictive set of flags is set in the result
// returns a new version that is a maximal intersection of the two ranges
function MaximizeRequiredRanges(lhs, rhs)
{
	var newVersion = new Object();
	
	// max version field
	if (compareVersions(lhs.Version, rhs.Version) < 0)
		newVersion.Version = rhs.Version;
	else
		newVersion.Version = lhs.Version;
	
	// 'max' OS flags by having set trump not set
	newVersion.Need64Bit = lhs.Need64Bit | rhs.Need64Bit;
	newVersion.NeedServer = lhs.NeedServer | rhs.NeedServer;
	
	return newVersion;
}


// the point intersection is empty if the versions don't match,
// and the more restrictive set of flags if the versions do match
// returns the intersection of the two versions, or null if they don't 'overlap'
function IntersectRequiredPointSamples(lhs, rhs)
{
	var result = null;
	
	if (0 == compareVersions(lhs.Version, rhs.Version))
	{
		result = new Object();
		result.Version = lhs.Version;
		result.Need64Bit = ((1 == lhs.Need64Bit) || (1 == rhs.Need64Bit)) ? 1 : 0;
		result.NeedServer = ((1 == lhs.NeedServer) || (1 == rhs.NeedServer)) ? 1 : 0;
	}
	
	return result;
}


// Adds the point sample to the set of point sample, unioning
// in the event of a collision.  This manipulates ioSet in place
function UnionRequiredPointSampleWithSet(ioSet, pointSample)
{
	var updatedSample = null;
	var overlappingSample = ioSet[pointSample.Version];
	if (overlappingSample)
	{
		updatedSample = new Object();
		updatedSample.Version = pointSample.Version;
		updatedSample.Need64Bit = ((1 == lhs.Need64Bit) && (1 == rhs.Need64Bit)) ? 1 : 0;
		updatedSample.NeedServer = ((1 == lhs.NeedServer) && (1 == rhs.NeedServer)) ? 1 : 0;
	}
	else
	{
		updatedSample = pointSample;
	}
	
	ioSet[pointSample.Version] = updatedSample;
}


// intersects two point sample sets, returning the intersetion
// the intersection could be an empty set, but it will never be null
function IntersectPointSampleSets(lhsSet, rhsSet)
{
	var result = new Object;
	
	for (var aKey in lhsSet)
	{
		var lhsValue = lhsSet[aKey];
		var rhsValue = rhsSet[aKey];
		if (lhsValue && rhsValue)
		{
			var intersectionValue = IntersectRequiredPointSamples(lhsValue, rhsValue);
			if (intersectionValue)
				result[intersectionValue.Version] = intersectionValue;
		}
	}
	
	return result;
}


// returns the requirement as a human comprehendable string
function RequireToString(aRequire)
{
	var result = "MacOS X"
				 + ((1 == aRequire.NeedServer) ? " Server" : "")
				 + " " + aRequire.Version
				 + ((1 == aRequire.Need64Bit) ? " 64-Bit" : "");
	return result;
}

/**
Forms a human-readible string describing the particular Windows OS variant, range, and 64-Bittedness
inSession is the session to use for localizing the text
inWindowsVariant is the Windows variant, such as XP, Vista, or Server2003
inServicePackRange is the InclusiveRange of supported service packs
inIs64Bit is a boolean indicating whether or not the OS is 64-Bit
*/
function DescribeWinOS(inWindowsVariant, inServicePackRange, inIs64Bit)
{
	var formatString = null;
	var replacements = new Object;
	replacements["Variant"] = inWindowsVariant;
	replacements["64BitWithLeadingSpace"] = inIs64Bit ? " 64-Bit" : "";
	
	if (null == inServicePackRange.start)
	{
		if (null == inServicePackRange.end)
		{
			// (null, null)
			// e.g. Windows XP 64-Bit
			// e.g. Windows [Variant][64BitWithLeadingSpace]
			formatString = new LocalizedString("systemPageWinOSAllSP", null, replacements);
		}
		else
		{
			// (null, inServicePackRange.end]
			// e.g. Windows XP 64-Bit through Service Pack 1
			// e.g. Windows [Variant][64BitWithLeadingSpace] through Service Pack [SP]
			replacements["SP"] = inServicePackRange.end;
			formatString = new LocalizedString("systemPageWinOSThroughSP", null, replacements);
		}
	}
	else
	{
		if (null == inServicePackRange.end)
		{
			// [inServicePackRange.start, null)
			// e.g. Windows XP 64-Bit Service Pack 8 and greater
			// e.g. Windows [Variant][64BitWithLeadingSpace] Service Pack [MinSP] and greater
			replacements["SP"] = inServicePackRange.start;
			formatString = new LocalizedString("systemPageWinOSFromSP", null, replacements);
		}
		else
		{
			// [inServicePackRange.start, inServicePackRange.end]
			if (inServicePackRange.start == inServicePackRange.end)
			{
				// e.g. Windows XP Service Pack 3
				// e.g. Windows [Variant][64BitWithLeadingSpace] Service Pack [SP]
				replacements["SP"] = inServicePackRange.start;
				formatString = new LocalizedString("systemPageWinOSExactlySP", null, replacements);
			}
			else
			{
				// e.g. Windows Vista 64-Bit Service Pack 2 through Service Pack 4
				// e.g. Windows [Variant][64BitWithLeadingSpace] Service Pack [MinSP] through Service Pack [MaxSP]
				replacements["MinSP"] = inServicePackRange.start;
				replacements["MaxSP"] = inServicePackRange.end;
				formatString = new LocalizedString("systemPageWinOSFromMinSPThroughMaxSP", null, replacements);
			}
		}
	}
	
	return formatString;
}
