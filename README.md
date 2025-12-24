# Immutable Container Guard

Система контроля неизменяемости контейнеров для курсовой работы по безопасности ОС.

## Требования
- Fedora 38+
- runC (опционально для тестового режима)
- openssl-libs

## Установка
```bash
rpmbuild -ba specs/immutable-guard.spec
sudo dnf install ./rpmbuild/RPMS/x86_64/immutable-guard-1.0-1.fc*.rpm
sudo systemctl enable --now immutable-guard

## Использование
```bash
sudo systemctl status immutable-guard
curl http://localhost:8080/health
sudo journalctl -u immutable-guard -f

## Автор
user-12-32

