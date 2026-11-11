#!/bin/bash

PickTarget() {
	local target=""
	if [[ -z "$target" ]]; then
		echo "Which target?"
		echo "R: Release"
		echo "D: Debug"
		read -r -p "> " target
	fi

	UDT_TARGET=""
	case "${target,,}" in
		r) UDT_TARGET=release ;;
		d) UDT_TARGET=debug ;;
		*)
			echo "Invalid choice: '$target'" >&2
			exit 1
			;;
	esac
}

PickClean() {
	local clean=""
	if [[ -z "$clean" ]]; then
		echo "Clean?"
		echo "Y: Yes"
		echo "N: No"
		read -r -p "> " clean
	fi

	UDT_CLEAN=0
	case "${clean,,}" in
		y) UDT_CLEAN=1 ;;
		n) UDT_CLEAN=0 ;;
		*)
			echo "Invalid choice: '$clean'" >&2
			exit 1
			;;
	esac
}
