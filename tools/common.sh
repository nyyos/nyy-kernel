#!/bin/sh

fetch ()
{
	local OPTIND=1
	local force_download=0
	local silet=0

	while getopts "fs" opt; do
		case "$opt" in
			f) force_download=1
				;;
			s) silent=1
				;;
		esac
	done

	shift $((OPTIND-1))
	[ "${1:-}" = "--" ] && shift

	if [[ $# -lt 2 ]]; then
		echo "invalid args!"
		echo "usage: fetch [-f] outfile url"
		return 1
	fi

	local output="$1"
	local url="$2"

	if [[ -f $output ]]; then
		if [[ ! $force_download -eq 1 ]]; then
			return
		fi
		rm -f "$output"
	fi

	local curl_args="-L"
	if [[ $silent -eq 0 ]]; then
		echo "downloading $(basename "${output}") ..."
		curl_args="$curl_args --progress-bar"
	else
		curl_args="$curl_args -s"
	fi
	curl ${curl_args} "${url}" -o "${output}"
}
