Name:           immutable-guard
Version:        1.0
Release:        1%{?dist}
Summary:        Immutable Container Guard for runC containers

License:        GPLv3+
URL:            https://github.com/flozent/immutable-guard
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  gcc
BuildRequires:  systemd-devel
BuildRequires:  openssl-devel
Requires:       openssl-libs
Requires:       systemd

%global debug_package %{nil}

%description
Daemon for monitoring immutability of containers. Detects changes
in container filesystems and stops compromised containers.

%prep
%setup -q

%build
make %{?_smp_mflags}

%install
rm -rf %{buildroot}
make install DESTDIR=%{buildroot}

%post
chown root:root %{_libexecdir}/immutable-guard-helper
if ! getent passwd user-12-32 > /dev/null 2>&1; then
    useradd -r -s /sbin/nologin -d /var/lib/immutable-guard user-12-32
fi
mkdir -p /var/lib/immutable-guard/hashes
chown user-12-32:user-12-32 /var/lib/immutable-guard -R
systemctl daemon-reload 2>/dev/null || :

%postun
if [ $1 -eq 0 ]; then
    systemctl disable immutable-guard.service 2>/dev/null || :
fi

%files
%doc README.md
%license LICENSE
%config(noreplace) /etc/immutable-guard.conf
%{_bindir}/immutable-guard
%{_libexecdir}/immutable-guard-helper
%{_unitdir}/immutable-guard.service
%dir /var/lib/immutable-guard
%dir /var/lib/immutable-guard/hashes

%changelog
* Wed Dec 03 2025 user-12-32 <burunduk.dddd@yandex.ru> - 1.0-1
- Первая версия пакета
