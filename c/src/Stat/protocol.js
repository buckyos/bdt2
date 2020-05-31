const Header = {
	protocolVersion: uint32,
	length: uint32,
	flags: uint32,
	magic: uint32,
	cmdType: uint32,
	timestamp: uint64,
	seq: uint32,
}

const req = {
	header: Header,
	source: {
		peerid: string,
		productid: string,
		productVersion: string,
		userProductid: string,
		userProductVersion: string,
	},
	body: {
	}
}

const resp = {
	header: Header,
	result: uint32,
}

const bdtSnapshotDelta = BdtStatisticsSnapshot;