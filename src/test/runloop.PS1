param([string]$build)

while ($True)
{
	.\RUNTESTS.PS1 -b $build -i libpmempool_backup -s TEST6

	if ($? -eq $False)
	{
		cat libpmempool_backup/pmem6.log
		echo "----------------------------------------------------"
		cat libpmempool_backup/pmempool6.log
		echo "---------------------------------------------------"
		cat libpmempoo_backup/prep6.log
		echo "~~~~~~~~~~~~~~!!!!!!!!!!!!!!!!!~~~~~~~~~~~~~~~~~~~~"
		Get-ChildItem "C:\temp\\test_libpmempool_backup6\pool.part4_backup" | % {[int]($_.length)}
		return
	}
}
