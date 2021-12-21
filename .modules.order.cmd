cmd_drivers/xuzhenhai/modules.order := {   cat drivers/xuzhenhai/hello/modules.order;   cat drivers/xuzhenhai/kasan-oob/modules.order; :; } | awk '!x[$$0]++' - > drivers/xuzhenhai/modules.order
