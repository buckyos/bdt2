

let exchange_key = {
	name:"ExchangeKey",
	id:"EXCHANGEKEY",
	field:[
		{seq:"uint32_t"},
		{seqAndKeySign:"uint8_t[BDT_SEQ_AND_KEY_SIGN_LENGTH]"},
		{fromPeerid:"BdtPeerid"},
		{peerInfo:"BdtPeerInfo*"},
		{sendTime:"uint64_t"}
	]
}


let syn_tunnel = {
	name:"SynTunnel",
	id:"SYN_TUNNEL",
	field:[
		{fromPeerid:"BdtPeerid"},
		{toPeerid:"BdtPeerid"},
		{proxyPeerid:"BdtPeerid"},
		{proxyPeerInfo:"BdtPeerInfo*"},
		{seq:"uint32_t"}, 
		{fromContainerId:"uint32_t"},
		{peerInfo:"BdtPeerInfo*"}, 
		{sendTime:"uint64_t"} 
		//BuckyString authInfo;

	]
}

let ack_tunnel = {
	name:"AckTunnel", 
	id:"ACK_TUNNEL",
	field:[
		{aesKeyHash:"uint64_t"},
		{seq:"uint32_t"},
		{fromContainerId:"uint32_t"},
        {toContainerId: "uint32_t"},
		{result:"uint8_t"},
		{peerInfo:"BdtPeerInfo*"},
		{sendTime:"uint64_t"},
		{mtu:"uint16_t"}
	]
}

let ackack_tunnel = {
	name:"AckAckTunnel",
	id:"ACKACK_TUNNEL",
	field:[
		{seq:"uint32_t"},
		{toContainerId:"uint32_t"}
	]
}

let ping_tunnel = {
	name:"PingTunnel",
	id:"PING_TUNNEL",
	field:[
		{toContainerId: "uint32_t"},
		{packageId:"uint32_t"},
		{sendTime:"uint64_t"},
		{recvData:"uint64_t"}
	]
}

let ping_tunnel_resp = {
	name:"PingTunnelResp",
	id:"PING_TUNNEL_RESP",
	field:[
		{toContainerId: "uint32_t"},
		{ackPackageId:"uint32_t"},
		{sendTime:"uint64_t"},
		{recvData:"uint64_t"}
	]
}

let call = {
	name:"SnCall",
	id:"SN_CALL",
	field:[
		{seq:"uint32_t"},
		{snPeerid:"BdtPeerid"},
		{toPeerid:"BdtPeerid"},
		{fromPeerid:"BdtPeerid"},
		{reverseEndpointArray:"BdtEndpointArray"},
		{peerInfo:"BdtPeerInfo*"},
		{payload:"BFX_BUFFER_HANDLE"}
	]
}

let call_resp = {
	name:"SnCallResp",
	id:"SN_CALL_RESP",
	field:[
		{seq:"uint32_t"},
		{snPeerid:"BdtPeerid"},
		{result:"uint8_t"},
		{toPeerInfo:"BdtPeerInfo*"}
	]
}

let called = {
	name:"SnCalled",
	id:"SN_CALLED",
	field:[
		{seq:"uint32_t"},
		{snPeerid:"BdtPeerid"},
		{toPeerid:"BdtPeerid"},
		{fromPeerid:"BdtPeerid"},
		{reverseEndpointArray:"BdtEndpointArray"}, 
		{peerInfo:"BdtPeerInfo*"},
		{payload:"BFX_BUFFER_HANDLE"}
	]
}

let called_resp = {
	name:"SnCalledResp",
	id:"SN_CALLED_RESP",
	field:[
		{seq:"uint32_t"},
		{snPeerid:"BdtPeerid"},
		{result:"uint8_t"}
	]
}

let sn_ping = {
	name : "SnPing",
	id:"SN_PING",
	field:[
		{seq:"uint32_t"},
		{snPeerid:"BdtPeerid"},
		{fromPeerid:"BdtPeerid"},
		{peerInfo:"BdtPeerInfo*"},
	]
}

let sn_ping_resp = {
	name : "SnPingResp",
	id:"SN_PING_RESP",
	field:[
		{seq:"uint32_t"},
		{snPeerid:"BdtPeerid"},
		{result:"uint8_t"},
		{peerInfo:"BdtPeerInfo*"},
		{endpointArray:"BdtEndpointArray"} 
	]
}

let data_gram = {
	name : "DataGram",
	id:"DATA_GRAM",
	field:[
		{seq:"uint32_t"},
		{destZone:"uint32_t"}, //TODO:因为DHT广播需要依赖点到点传输的字段。
		{hopLimit:"uint8_t"},
		{piece:"uint16_t"},
		{ackSeq:"uint32_t"},
		{sendTime:"uint64_t"},
		{authorPeerid:"BdtPeerid"},
		{authorPeerInfo:"BdtPeerInfo*"},
		{dataSign:"TinySignBytes"},
		{innerCmdType:"uint8_t"},
		{data:"BFX_BUFFER_HANDLE"}
		//{data:"list<BYTE>"}//TODO:
	]
}

let session_data = {
	name : "SessionData",
	id:"SESSION_DATA",
	field:[
		{sessionId:"uint32_t"},
		{streamPos:"uint48_t"},

		{synSeq:"uint32_t"},
		{synControl:"uint32_t"},
		{ccType:"uint8_t"},
		{toVPort:"uint16_t"},
		{fromPeerId:"BdtPeerid"},
		{toPeerId:"BdtPeerid"},
		{fromSessionId:"uint32_t"},

		{packageId:"uint32_t"},
		{totalRecv:"uint48_t"},

		{ackStreamPos:"uint48_t"},
		{toSessionId:"uint32_t"},
		{recvSpeedlimit:"uint32_t"},
		{sendTime:"uint32_t"},
		
	]
}

let tcp_syn_connection = {
	name : "TcpSynConnection",
	id:"TCP_SYN_CONNECTION",
	field:[
		{seq:"uint32_t"},
		{result:"uint8_t"},
		{toVPort:"uint16_t"},
		{fromSessionId:"uint32_t"},
		{fromPeerid:"BdtPeerid"},
		{toPeerid:"BdtPeerid"},
		{proxyPeerid:"BdtPeerid"},
		{peerInfo:"BdtPeerInfo*"},
		{reverseEndpointArray:"BdtEndpointArray"},
		{payload:"BFX_BUFFER_HANDLE"}
		//authInfo	
	]
}

let tcp_ack_connection = {
	name : "TcpAckConnection",
	id:"TCP_ACK_CONNECTION",
	field:[
		{seq:"uint32_t"},
		{toSessionId:"uint32_t"},
		{result:"uint8_t"},
		{peerInfo:"BdtPeerInfo*"},
		{payload:"BFX_BUFFER_HANDLE"}
	]
}

let tcp_ackack_connection = {
	name:"TcpAckAckConnection",
	id:"TCP_ACKACK_CONNECTION",
	field:[
		{seq:"uint32_t"},
		{result:"uint8_t"},
	]
}




function getReadFunNameByType(typeName,fieledName)
{
	if(typeName == "uint8_t")
	{
		return "BfxBufferReadUInt8(bufferStream, &(pResult->" + fieledName+"))";
	}

	if(typeName == "uint16_t")
	{
		return "BfxBufferReadUInt16(bufferStream, &(pResult->" + fieledName+"))";
	}

	if(typeName == "uint32_t")
	{
		return "BfxBufferReadUInt32(bufferStream, &(pResult->" + fieledName+"))";
	}

	if(typeName == "BdtPeerid")
	{
		return "Bdt_BufferReadPeerid(bufferStream, &(pResult->" + fieledName+"))";
	}

	if(typeName == "BdtPeerInfo*")
	{
		return "BufferReadPeerInfo(bufferStream, &(pResult->" + fieledName+"))";
	}

	if(typeName == "uint64_t")
	{
		return "BfxBufferReadUInt64(bufferStream, &(pResult->" + fieledName+"))";
	}

	if(typeName == "uint48_t")
	{
		return "BfxBufferReadUInt48(bufferStream, &(pResult->" + fieledName+"))";
	}

	if(typeName == "AesKeyBytes")
	{
		return "BfxBufferReadByteArray(bufferStream, pResult->" + fieledName+",32)";
	}

	if(typeName == "TinySignBytes")
	{
		return "BfxBufferReadByteArray(bufferStream, pResult->" + fieledName+",16)";
	}

	if(typeName == "BdtEndpointArray")
	{
		return "Bdt_BufferReadEndpointArray(bufferStream, &(pResult->" + fieledName + "))";
	}

	if(typeName == "BFX_BUFFER_HANDLE")
	{
		return "BfxBufferReadByteArrayBeginWithU16Length(bufferStream,&(pResult->" + fieledName +"))";
	}
}


function getWriteFunNameByType(typeName,fieledName)
{
    if (typeName == "uint8_t") {
        return "BfxBufferWriteUInt8(pStream, (pPackage->" + fieledName + "),&writeBytes);\n";
    }

    if (typeName == "uint16_t") {
        return "BfxBufferWriteUInt16(pStream, (pPackage->" + fieledName + "),&writeBytes);\n";
    }

    if (typeName == "uint32_t") {
        return "BfxBufferWriteUInt32(pStream, (pPackage->" + fieledName + "),&writeBytes);\n";
    }

    if (typeName == "BdtPeerid") {
        return "Bdt_BufferWritePeerid(pStream, &(pPackage->" + fieledName + "),&writeBytes);\n";
    }

    if (typeName == "BdtPeerInfo*") {
        return "BufferWritePeerInfo(pStream, (pPackage->" + fieledName + "),&writeBytes);\n";
    }

    if (typeName == "uint64_t") {
        return "BfxBufferWriteUInt64(pStream, (pPackage->" + fieledName + "),&writeBytes);\n";
	}
	
	if (typeName == "uint64_t") {
        return "BfxBufferWriteUInt48(pStream, (pPackage->" + fieledName + "),&writeBytes);\n";
	}

	if(typeName == "AesKeyBytes")
	{
		return "BfxBufferWriteByteArray(pStream, pPackage->" + fieledName+",32);" + `
		writeBytes = 32;
		`;
	}

	if(typeName == "TinySignBytes")
	{
		return "BfxBufferWriteByteArray(pStream, pPackage->" + fieledName+",16);" + `
		writeBytes = 16;
		`;		
	}

	if(typeName == "BdtEndpointArray")
	{
		return "Bdt_BufferWriteEndpointArray(pStream,&(pPackage->" + fieledName + "),&writeBytes);\n";

	}

	if(typeName == "BFX_BUFFER_HANDLE")
	{
		return "BfxBufferWriteByteArrayBeginWithU16Length(pStream,(pPackage->" + fieledName + "),&writeBytes);\n"
	}

	return "//TODO:\n"
}


function getCopyFunNameByType(typeName,packageType,filedName) {
	let  t = "";
	if(typeName == "uint8_t[BDT_SEQ_AND_KEY_SIGN_LENGTH]")
	{
		t = `memcpy(pFieldValue,p%PACKAGE_NAME->%FIELD_ID,BDT_SEQ_AND_KEY_SIGN_LENGTH);`;
	}
	else if(typeName == "BdtPeerInfo*")
	{
		 t =  `(*(const BdtPeerInfo**)pFieldValue) = p%PACKAGE_NAME->%FIELD_ID;
				  BdtAddRefPeerInfo(p%PACKAGE_NAME->%FIELD_ID);	
				`
	} else if (typeName == "AesKeyBytes")
	{
		t = `memcpy(pFieldValue,p%PACKAGE_NAME->%FIELD_ID,32);`;
	}else if (typeName == "TinySignBytes")
	{
		t = `memcpy(pFieldValue,p%PACKAGE_NAME->%FIELD_ID,16);`;
	}
	else if(typeName == "BdtEndpointArray")
	{
		t = "BdtEndpointArrayClone(&(p%PACKAGE_NAME->%FIELD_ID),pFieldValue);";
	}
	else if(typeName == "BFX_BUFFER_HANDLE")
	{
		t = "assert(0);";
	}
	else
	{
		t = `(*(%FIELD_TYPE*)pFieldValue) = p%PACKAGE_NAME->%FIELD_ID;`;
	}

	return replaceStr(t,{"FIELD_TYPE":typeName,"PACKAGE_NAME":packageType,"FIELD_ID":filedName});
}

function getEqualCodeByType(field) {
	let typeName = field.type;
	if(typeName == "uint8_t")
	{
		return "(pPackage->" + field.name + " == *rParam)";
	}

	if(typeName == "uint16_t")
	{
		return "(pPackage->" + field.name + " == *rParam)";
	}

	if(typeName == "uint32_t")
	{
		return "(pPackage->" + field.name + " == *rParam)";
	}

	if(typeName == "BdtPeerid")
	{
		return "(memcmp(rParam->pid, pPackage->" + field.name + ".pid, BDT_PEERID_LENGTH) ==0);"
	}

	if(typeName == "BdtPeerInfo*")
	{
		return "BdtPeerInfoIsEqual(pPackage->" + field.name + ",*rParam);"
	}

	if(typeName == "uint64_t")
	{
		return "(pPackage->" + field.name + " == *rParam)";
	}

	if(typeName == "AesKeyBytes")
	{
		return "(memcmp(rParam, pPackage->" + field.name + ", 32) ==0);"
	}

	if(typeName == "TinySignBytes")
	{
		return "(memcmp(rParam, pPackage->" + field.name + ", 16) ==0);"
	}

	if(typeName == "BdtEndpointArray")
	{
		return "BdtEndpointArrayIsEqual(rParam,&(pPackage->" + field.name + "));"
	}

	if(typeName == "BFX_BUFFER_HANDLE")
	{
		return "false;"
	}

	return "(pPackage->" + field.name + " == *rParam)";
}

function getIsDefaultCode(filedName,typeName)
{
	if(typeName == "uint8_t")
	{
		return "(" + filedName + " == 0);";
	}

	if(typeName == "uint16_t")
	{
		return "(" + filedName + " == 0);";
	}

	if(typeName == "uint32_t")
	{
		return "(" + filedName + " == 0);";
	}

	if(typeName == "BdtPeerid")
	{
		return "(memcmp(" + filedName + ".pid,defaultPid, BDT_PEERID_LENGTH) ==0);"
	}

	if(typeName == "BdtPeerInfo*")
	{
		return "(" + filedName + " == NULL);";
	}

	if(typeName == "uint64_t")
	{
		return "(" + filedName + " == 0);";
	}

	if(typeName == "uint48_t")
	{
		return "(" + filedName + " == 0);";
	}

	if(typeName == "AesKeyBytes")
	{
		return "(memcmp(" + filedName + ",defaultBytes32, 32) ==0);"
	}

	if(typeName == "TinySignBytes")
	{
		return "(memcmp(" + filedName + ",defaultBytes16, 16) ==0);"
	}

	if(typeName == "BdtEndpointArray")
	{
		return "(" + filedName + ".size == 0);"
	}

	if(typeName == "BFX_BUFFER_HANDLE")
	{
		return "(" + filedName + " == NULL);";
	}

	return "false;";
}

var allField = {};
function getAllField(packagedefines) {
	allField = {};
	for(var k=0;k<packagedefines.length;++k)
	{
		let v = packagedefines[k];
		for(var fid=0;fid<v.field.length;++fid) {
			let field = getFieldIDAndType(v.field[fid]);
			allField[field.name] = field.type;
		}
	}
}

function replaceStr(content,env) {
	for(var k in env) {
		v = env[k];
		//console.log(k,v)
		content = content.replace(new RegExp("\%"+k,"g"),v)
	}
	return content;
}

function getFieldIDAndType(field)
{
	for(var k in field)
	{
		v = field[k];
		return {"name":k,"type":v}
	}
}

function genPackageDefine(packagedefines) {
	let c="";
	for(var k=0;k<packagedefines.length;++k)
	{
		
		let v = packagedefines[k];
		for(var fid=0;fid<v.field.length;++fid) {
			let field = getFieldIDAndType(v.field[fid]);
			let def = `
#define PACKAGE_%BDT_ID_PACKAGE_FLAG_%FIELD_ID (1<<%POS)`
			c = c + replaceStr(def,{"PACKAGE_ID":v.id,"FIELD_ID":field.name.toUpperCase(),"POS":fid});
		}

		def = `
typedef struct Bdt_Package%PACKAGE_NAME {
	uint8_t cmdtype;
	uint16_t cmdflags;`
		c = c + replaceStr(def,{"PACKAGE_NAME":v.name})

		for(var fid=0;fid<v.field.length;++fid) {
			let field = getFieldIDAndType(v.field[fid]);
			let def = `
	%FIELD_TYPE %FIELD_ID;`
			c = c + replaceStr(def,{"FIELD_ID":field.name,"FIELD_TYPE":field.type});
		}
		def = `
} Bdt_Package%PACKAGE_NAME;
Bdt_%PACKAGE_NAMEPackage* Bdt_%PACKAGE_NAMEPackageCreate();
void Bdt_%PACKAGE_NAMEPackageInit(Bdt_%PACKAGE_NAMEPackage* package);
void Bdt_%PACKAGE_NAMEPackageFinalize(Bdt_%PACKAGE_NAMEPackage* package);
void Bdt_%PACKAGE_NAMEPackageFree(Bdt_%PACKAGE_NAMEPackage* package);
`
		c = c + replaceStr(def,{"PACKAGE_NAME":v.name})
	}

	return c;
}


function genDecodeSignlePackage(packagedefines) {
	let header = `
//THIS FUNCTION GENERATE BY packages.js
static Bdt_Package* Bdt_DecodeSinglePackage(uint8_t cmdtype, uint16_t totallen, BfxBufferStream* bufferStream, Bdt_Package* pRefPackage)
{

	int r = 0;
`
	let c = header;
	for(var k in packagedefines)
	{
		v = packagedefines[k];
        let def = `	Bdt_%PACKAGE_NAMEPackage* p%PACKAGE_NAMEPackage = NULL;
`
		c = c + replaceStr(def,{"PACKAGE_NAME":v.name})
	}
	c = c + 
`
	switch (cmdtype)
	{`
	for(var k=0;k<packagedefines.length;++k)
	{
		let v = packagedefines[k];
		let case_code = `
	case BDT_%PACKAGE_ID_PACKAGE:
		p%PACKAGE_NAMEPackage = Bdt_%PACKAGE_NAMEPackageCreate();
		r = Decode%PACKAGE_NAMEPackage(p%PACKAGE_NAMEPackage, totallen, bufferStream, pRefPackage);
		if (r == BFX_RESULT_SUCCESS)
		{
			return (Bdt_Package*)p%PACKAGE_NAMEPackage;
		}
		else
		{
			Bdt_%PACKAGE_NAMEPackageFree(p%PACKAGE_NAMEPackage);
			return NULL;
		}
		break;	
	`
		c = c + replaceStr(case_code,{"PACKAGE_NAME":v.name,"PACKAGE_ID":v.id})
	}

	c += `
	}
	return NULL;
}
	`

	return c;

}



function genDecodePackageFunctions(packagedefines){
	let c = "";
	let def = `
//THIS FUNCTION GENERATE BY packages.js	
static int Decode%PACKAGE_NAMEPackage(Bdt_%PACKAGE_NAMEPackage* pResult, uint16_t totallen, BfxBufferStream* bufferStream, Bdt_Package* pRefPackage)
{
	int r = 0;
	size_t minsize = Bdt_GetPackageMinSize(BDT_%PACKAGE_ID_PACKAGE);
	uint16_t cmdflags;
	int readlen = 0;

	if (totallen < minsize)
	{
		BLOG_WARN("decode BDT_%PACKAGE_ID_PACKAGE package error,totallen(%hu) too small.", totallen);
		goto END;
	}

	r = BfxBufferReadUInt16(bufferStream, &(cmdflags));
	if (r <= 0)
	{
		BLOG_WARN("decode BDT_%PACKAGE_ID_PACKAGE error,cann't read cmdflags.");
		goto END;
	}
	readlen += r;
	pResult->cmdflags = cmdflags;
	bool isDisableRef = cmdflags & BDT_PACKAGE_FLAG_DISABLE_REF;
	if (isDisableRef)
	{
		pRefPackage = NULL;
	}
	`
	for(var k=0;k<packagedefines.length;++k) {
		var v = packagedefines[k];
		c = c +  replaceStr(def,{"PACKAGE_NAME":v.name,"PACKAGE_ID":v.id})

		for(var fid=0;fid<v.field.length;++fid) {
			let field = getFieldIDAndType(v.field[fid]);
			let def = `
	bool haveField_%FIELD_ID = cmdflags & BDT_%PACKAGE_ID_PACKAGE_FLAG_%FIELD_UP;
	if (haveField_%FIELD_ID)
	{
		r = %READ_FUNCTION;
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_%PACKAGE_ID_PACKAGE error,cann't read %FIELD_ID.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_%PACKAGE_ID_PACKAGE_FLAG_%FIELD_UP, pRefPackage,(void*)&(pResult->%FIELD_ID));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_%PACKAGE_ID_PACKAGE:field %FIELD_ID read from refPackage.");
			}
		}
	}`
			let readFunName = getReadFunNameByType(field.type,field.name);
			c = c + replaceStr(def,{
				"PACKAGE_NAME":v.name,
				"PACKAGE_ID":v.id,
				"FIELD_ID":field.name,
				"FIELD_UP":field.name.toUpperCase(),
				"READ_FUNCTION":readFunName});
		}

		let footer = `
	r = BFX_RESULT_SUCCESS;
END:
	if (totallen != readlen)
	{
		BLOG_WARN("decode BDT_%PACKAGE_ID_PACKAGE error,totallen != readlen.");
		r = -1;
	}
	return r;
}`;		
		c = c +  replaceStr(footer,{"PACKAGE_NAME":v.name,"PACKAGE_ID":v.id})

	}

	return c;
}

function genFiledTransStaticDefine(packagedefs)
{
	let c = "";
	getAllField(packagedefs);
	let t = `
static const char* s_fieldName_%FIELD_ID = "%FIELD_ID";`
	for(var k in allField)
	{
		c = c + replaceStr(t,{"FIELD_ID":k});
	}
	return c;
}



function genIsEqualPackageFieldDefaultValue(packagedefs)
{
	let c = "";
	let t = `
//THIS FUNCTION GENERATE BY packages.js	
static bool IsEqualPackageFieldDefaultValue(const Bdt_Package* package, uint16_t fieldFlags)
{	
	bool r = false;
	static uint8_t defaultPid[BDT_PEERID_LENGTH] = {0};
	static uint8_t defaultBytes16[16] = {0};
	static uint8_t defaultBytes32[32] = {0};
	`;
	c += t;

	for(var k=0;k<packagedefs.length;++k)
	{
		var v = packagedefs[k];
		c += replaceStr(
`
	const Bdt_%PACKAGE_NAMEPackage* p%PACKAGE_NAME = NULL;`,{"PACKAGE_NAME":v.name});
	}

	c += `
	switch (package->cmdtype)
	{
	`;

	for(var k=0;k<packagedefs.length;++k)
	{
		let v = packagedefs[k];
		t = `	
	case BDT_%PACKAGE_ID_PACKAGE:
		p%PACKAGE_NAME = (Bdt_%PACKAGE_NAMEPackage*)package;
		switch (fieldFlags)
		{`
		c = c + replaceStr(t,{"PACKAGE_ID":v.id,"PACKAGE_NAME":v.name});
		for(var fid=0;fid<v.field.length;++fid) {
			let field = getFieldIDAndType(v.field[fid]);
			let case_code = `
		case BDT_%PACKAGE_ID_PACKAGE_FLAG_%FIELD_UP:
			r = %IS_DEFAULT_CODE
			break;`;
			let is_default_code =getIsDefaultCode("p" + v.name + "->" + field.name,field.type);
			c = c + replaceStr(case_code,{"PACKAGE_ID":v.id,"FIELD_UP":field.name.toUpperCase(),"FIELD_ID":field.name,"IS_DEFAULT_CODE":is_default_code});
		}
		c += `
		}
		break;`
	}		
	c += `
	}
	return r;
}
	`
	return c;
}

function genGetPackageFieldName(packagedefs)
{
	let t = `
//THIS FUNCTION GENERATE BY packages.js	
static inline const char* GetPackageFieldName(uint8_t cmdtype, uint16_t fieldFlags)
{
	switch (cmdtype)
	{`;
	let c = "";
	c = t;
	for(var k=0;k<packagedefs.length;++k)
	{
		let v = packagedefs[k];
		t = `	
	case BDT_%PACKAGE_ID_PACKAGE:
		switch (fieldFlags)
		{`
		c = c + replaceStr(t,{"PACKAGE_ID":v.id});
		for(var fid=0;fid<v.field.length;++fid) {
			let field = getFieldIDAndType(v.field[fid]);
			let casecase = `
		case BDT_%PACKAGE_ID_PACKAGE_FLAG_%FIELD_UP:
			return s_fieldName_%FIELD_ID;`;
			c = c + replaceStr(casecase,{"PACKAGE_ID":v.id,"FIELD_UP":field.name.toUpperCase(),"FIELD_ID":field.name});
		}
		c += `
		}
		break;`
	}	

	c += `
	}
	assert(0);
	return NULL;
}`
	return c;
}


function genGetPackageValueByName(packagedefs){
	let header = `
//THIS FUNCTION GENERATE BY packages.js	
static int GetPackageFieldValueByName(const Bdt_Package* pPackage,const char* fieldName,void* pFieldValue)
{	
	`;
	let c = header;
	for(var k=0;k<packagedefs.length;++k)
	{
		var v = packagedefs[k];
        c += replaceStr("const Bdt_%PACKAGE_NAMEPackage* p%PACKAGE_NAME = NULL;\n",{"PACKAGE_NAME":v.name});
	}
	c += `
	switch (pPackage->cmdtype)
	{	
	`;
	for(var k=0;k<packagedefs.length;++k)
	{
		var v = packagedefs[k];
		c += replaceStr(`
	case BDT_%PACKAGE_ID_PACKAGE:
		p%PACKAGE_NAME = (Bdt_%PACKAGE_NAMEPackage*)pPackage;
		`,{"PACKAGE_NAME":v.name,"PACKAGE_ID":v.id});
		for(var fid=0;fid<v.field.length;++fid) {
			let field = getFieldIDAndType(v.field[fid]);
			ifcode = `
		if (fieldName == s_fieldName_%FIELD_ID)
		{
			%COPY_CODE
			return BFX_RESULT_SUCCESS;
		}`;
			let copycode = getCopyFunNameByType(field.type,v.name,field.name);
			c += replaceStr(ifcode,{"PACKAGE_NAME":v.name,"FIELD_ID":field.name,"COPY_CODE":copycode});
		}
		c += `
		break;`
	}	
	c += `
	}
	return BFX_RESULT_NOT_FOUND;
}`
	return c;
}

function genGetPackageValuePointByName(packagedefs){
	let header = `
//THIS FUNCTION GENERATE BY packages.js	
static const void* GetPackageFieldPointByName(const Bdt_Package* pPackage, const char* fieldName)
{	
	`;
	let c = header;
	for(var k=0;k<packagedefs.length;++k)
	{
		var v = packagedefs[k];
        c += replaceStr("const Bdt_%PACKAGE_NAMEPackage* p%PACKAGE_NAME = NULL;\n",{"PACKAGE_NAME":v.name});
	}
	c += `
	switch (pPackage->cmdtype)
	{	
	`;
	for(var k=0;k<packagedefs.length;++k)
	{
		var v = packagedefs[k];
		c += replaceStr(`
	case BDT_%PACKAGE_ID_PACKAGE:
		p%PACKAGE_NAME = (Bdt_%PACKAGE_NAMEPackage*)pPackage;
		`,{"PACKAGE_NAME":v.name,"PACKAGE_ID":v.id});
		for(var fid=0;fid<v.field.length;++fid) {
			let field = getFieldIDAndType(v.field[fid]);
			ifcode = `
		if (fieldName == s_fieldName_%FIELD_ID)
		{
			return &(p%PACKAGE_NAME->%FIELD_ID);
		}`;
			let copycode = getCopyFunNameByType(field.type,v.name,field.name);
			c += replaceStr(ifcode,{"PACKAGE_NAME":v.name,"FIELD_ID":field.name,"COPY_CODE":copycode});
		}
		c += `
		break;`
	}	
	c += `
	}
	return NULL;
}`
	return c;
}


function genEncodeSinglePackage(packagedefines)
{
	let c = `
//THIS FUNCTION GENERATE BY packages.js	
int Bdt_EncodeSinglePackage(Bdt_Package* package, const Bdt_Package* refPackage,BfxBufferStream* pStream, size_t* pWriteBytes)
{

	switch (package->cmdtype)
	{`
	for(var k=0;k<packagedefines.length;++k)
	{
		var v = packagedefines[k];
		let case_code = `
	case BDT_%PACKAGE_ID_PACKAGE:
		return Encode%PACKAGE_NAMEPackage(package,refPackage, pStream, pWriteBytes);
		break;`
		c += replaceStr(case_code,{"PACKAGE_ID":v.id,"PACKAGE_NAME":v.name});
	}
	c += `
	}
	BLOG_WARN("Encode SinglePackage:unknown package,cmdtype=%d", package->cmdtype);
	return BFX_RESULT_UNKNOWN_TYPE;
}	
	`
	return c;
}

function genEncodeFunctions(packagedefines)
{
	let c = "";
	for(var k=0;k<packagedefines.length;++k)
	{
		var v = packagedefines[k];
		let header_code = `
//THIS FUNCTION GENERATE BY packages.js
static int Encode%PACKAGE_NAMEPackage(Bdt_Package* package, const Bdt_Package* refPackage, BfxBufferStream* pStream, size_t* pWriteBytes)
{
	Bdt_%PACKAGE_NAMEPackage* pPackage = (Bdt_%PACKAGE_NAMEPackage*)package;
	size_t writeBytes = 0;
	bool isEqual = false;
	bool haveRef = false;
	size_t totalWriteBytes = 0;
	int r = 0;
	*pWriteBytes = 0;

	if (package == refPackage)
	{
		refPackage = NULL;
	}
	if (pPackage->cmdflags & BDT_PACKAGE_FLAG_DISABLE_REF)
	{
		refPackage = NULL;
	}

	size_t headerPos = BfxBufferStreamGetPos(pStream);
	
	//write cmdtype,totallen,cmdflags
	BfxBufferStreamSetPos(pStream,headerPos + 1 + 2 + 2);
	totalWriteBytes += 5;		
	`
		c += replaceStr(header_code,{"PACKAGE_ID":v.id,"PACKAGE_NAME":v.name})
		for(var fid=0;fid<v.field.length;++fid)
		{
			let field = getFieldIDAndType(v.field[fid]);
			let write_code = `
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_%PACKAGE_ID_PACKAGE_FLAG_%FIELD_UP);
		if (fieldName)
		{	
			%FIELD_TYPE* rParam = (%FIELD_TYPE*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = %EQUAL_CODE;
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_%PACKAGE_ID_PACKAGE_FLAG_%FIELD_UP);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_%PACKAGE_ID_PACKAGE_FLAG_%FIELD_UP);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_%PACKAGE_ID_PACKAGE_FLAG_%FIELD_UP))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_%PACKAGE_ID_PACKAGE_FLAG_%FIELD_UP);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_%PACKAGE_ID_PACKAGE_FLAG_%FIELD_UP);
		}
	}

	if (pPackage->cmdflags & BDT_%PACKAGE_ID_PACKAGE_FLAG_%FIELD_UP)
	{
		//write to buffer
		r = %WRITE_CODE
		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	`
		  let writefun = getWriteFunNameByType(field.type,field.name);
		  let equalCode = getEqualCodeByType(field);
		  c += replaceStr(write_code,{"PACKAGE_ID":v.id,"PACKAGE_NAME":v.name,
		   "FIELD_ID":field.name,"WRITE_CODE":writefun,"FIELD_UP":field.name.toUpperCase(),
           "FIELD_TYPE":field.type,"EQUAL_CODE":equalCode});
		}

		c += `
	size_t tailPos = BfxBufferStreamGetPos(pStream);
	BfxBufferStreamSetPos(pStream, headerPos);
	BfxBufferWriteUInt16(pStream, (uint16_t)totalWriteBytes,&writeBytes);
	BfxBufferWriteUInt8(pStream, pPackage->cmdtype,&writeBytes);
	BfxBufferWriteUInt16(pStream, pPackage->cmdflags,&writeBytes);
	BfxBufferStreamSetPos(pStream, tailPos);
	*pWriteBytes = totalWriteBytes;
	
	return BFX_RESULT_SUCCESS;
}	

		`
	}
	return c;
}

let allpackages = [exchange_key,syn_tunnel,ack_tunnel,ackack_tunnel,ping_tunnel,ping_tunnel_resp,call,call_resp,called,called_resp,sn_ping,sn_ping_resp,data_gram,tcp_syn_connection,tcp_ack_connection,tcp_ackack_connection];
let allpackages_with_datasession =  [exchange_key,syn_tunnel,ack_tunnel,ackack_tunnel,ping_tunnel,ping_tunnel_resp,call,call_resp,called,called_resp,sn_ping,sn_ping_resp,data_gram,tcp_syn_connection,tcp_ack_connection,tcp_ackack_connection,session_data];
let packagedefs = [syn_tunnel,ack_tunnel,ackack_tunnel,ping_tunnel,ping_tunnel_resp,call,call_resp,called,called_resp,sn_ping,sn_ping_resp,data_gram,tcp_syn_connection,tcp_ack_connection,tcp_ackack_connection];
let encodedecodepackage = [syn_tunnel,ack_tunnel,ackack_tunnel,ping_tunnel,ping_tunnel_resp,call,call_resp,called,called_resp,sn_ping,sn_ping_resp,tcp_syn_connection,tcp_ack_connection,tcp_ackack_connection];
let encodedecodepackage_with_datasession =  [syn_tunnel,ack_tunnel,ackack_tunnel,ping_tunnel,ping_tunnel_resp,call,call_resp,called,called_resp,sn_ping,sn_ping_resp,data_gram,tcp_syn_connection,tcp_ack_connection,tcp_ackack_connection,session_data];
//console.log(genPackageDefine(allpackages));






function genPackageImp(){
	var fs = require('fs'); 
	function writeFile(file, content){ 
		
      
		// appendFile，如果文件不存在，会自动创建新文件  
		// 如果用writeFile，那么会删除旧文件，直接写新文件  
		fs.appendFile(file, "\r\n" + content, function(err){  
			if(err)  
				console.info("fail " + err);  
			else  
				console.info("写入文件ok");  
		});  
	}  

	console.log = function(msg){
	  //process.stdout.write('Log data: ' + msg);
	  writeFile('PackageImp.imp.inl', msg);
	};

	fs.writeFile("PackageImp.imp.inl","", (err) => {
		if (err) throw err;
		console.log('');
	  });

	console.log(`
#include "./Package.h"
#include "./PackageDecoder.h"
#include "./PackageEncoder.h"

#include <assert.h>
	`);

	console.log(genFiledTransStaticDefine(allpackages));
	console.log(genGetPackageFieldName(allpackages));
	console.log(genIsEqualPackageFieldDefaultValue(packagedefs));
	console.log(genGetPackageValueByName(allpackages));
	console.log(genGetPackageValuePointByName(allpackages));	

	console.log(genEncodeFunctions(encodedecodepackage));
	console.log(genEncodeSinglePackage(allpackages_with_datasession));

	console.log(genDecodePackageFunctions(encodedecodepackage));
	console.log(genDecodeSignlePackage(encodedecodepackage_with_datasession));
}

genPackageImp();
//console.log(genPackageDefine(packagedefs));

//console.log(genEncodeFunctions(encodedecodepackage));
//console.log(genEncodeSinglePackage(allpackages));

//console.log(genDecodePackageFunctions(encodedecodepackage));
//console.log(genDecodeSignlePackage(encodedecodepackage));