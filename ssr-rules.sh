#!/bin/sh
. /lib/functions.sh

flush_rules() {
	iptables-save -c | grep -v "SSR_SPEC" | sed '/[^ ]--/d' | uniq | iptables-restore -c
	if command -v ip >/dev/null 2>&1; then
		ip rule del fwmark 1 lookup 100 2>/dev/null
		ip route del local default dev lo table 100 2>/dev/null
	fi
	for setname in $(ipset -n list | grep "ssr_spec"); do
		ipset destroy $setname 2>/dev/null
	done
	return 0
}

ipset_init() {
	ipset -! restore <<-EOF || return 1
		create ssr_spec_src_direct hash:net hashsize 64
		create ssr_spec_src_forward hash:net hashsize 64
		create ssr_spec_dst_direct hash:net hashsize 64
		create ssr_spec_dst_manual hash:net hashsize 64
		create ssr_spec_dst_gfwlist hash:ip hashsize 64
		create ssr_spec_dst_others hash:net hashsize 64
EOF
	return 0
}

ipt_nat() {
	local ipt="iptables -t nat"
    local cross=$1
	local listen_port=$2
	$ipt -N SSR_SPEC_CROSS || return 1
	$ipt -A SSR_SPEC_CROSS -p tcp -j REDIRECT --to-ports $listen_port || return 1
	include_ac_rules nat $cross || return 1
	$ipt -A PREROUTING -j SSR_SPEC_LAN_AC || return 1
}

ipt_mangle() {
	local ipt="iptables -t mangle"
    local cross=$1
	local listen_port=$2
	$ipt -N SSR_SPEC_CROSS || return 1
	$ipt -A SSR_SPEC_CROSS -p udp -j TPROXY --on-port $listen_port --tproxy-mark 0x01/0x01 || return 1
	ip rule add fwmark 1 lookup 100 || return 1
	ip route add local default dev lo table 100 || return 1
	include_ac_rules mangle $cross || return 1
	$ipt -A PREROUTING -j SSR_SPEC_LAN_AC || return 1
}

include_ac_rules() {
	local ipt_chain=$1
    local ipt_policy=$2
	local proto=$([ "$ipt_chain" = "mangle" ] && echo udp || echo tcp)
	iptables-restore -n <<-EOF
	*$ipt_chain
	:SSR_SPEC_LAN_AC - [0:0]
	:SSR_SPEC_WAN_FW - [0:0]
	-A SSR_SPEC_LAN_AC -m set --match-set ssr_spec_src_direct src -j RETURN
	-A SSR_SPEC_LAN_AC -m set --match-set ssr_spec_src_forward src -j SSR_SPEC_WAN_FW
	-A SSR_SPEC_WAN_FW -m set --match-set ssr_spec_dst_direct dst -j RETURN
	-A SSR_SPEC_WAN_FW -m set --match-set ssr_spec_dst_manual dst -j SSR_SPEC_CROSS
	-A SSR_SPEC_WAN_FW -m set --match-set ssr_spec_dst_gfwlist dst -j SSR_SPEC_CROSS
	-A SSR_SPEC_WAN_FW -p $proto -j $ipt_policy
	COMMIT
EOF
}

flush_rules
ipset_init
ipt_nat
ipt_mangle
