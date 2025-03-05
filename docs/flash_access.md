## udev rules
There is a udev rule that links `/dev/mtdpart_${partitionname}` back to
`/dev/mtdpartX`. This allows re-ordering of partitions without breaking
scripts. Likewise `/dev/mtdpartblock_${partitionname}` links to
`/dev/mtdblockX`
