| Comando | Descripción |
|---------|-------------|
| `show ip interface brief` | Muestra un resumen del estado de las interfaces. |
| `show interfaces FastEthernet0/0` | Muestra detalles completos de una interfaz. |
| `show running-config interface FastEthernet0/0` | Muestra la configuración actual de una interfaz específica. |
| `configure terminal` | Entra al modo de configuración global. |
| `interface FastEthernet0/0` | Entra a la configuración de una interfaz específica. |
| `ip address 192.168.1.1 255.255.255.0` | Asigna una dirección IP a la interfaz. |
| `no shutdown` | Activa la interfaz. |
| `shutdown` | Desactiva la interfaz. |
| `speed 100` | Configura la interfaz para funcionar a 100 Mbps. |
| `duplex full` | Configura la interfaz para funcionar en dúplex completo. |
| `description Conexión a la red LAN principal` | Agrega una descripción a la interfaz. |
| `copy running-config startup-config` | Guarda la configuración actual para que se mantenga tras un reinicio. |
| `show startup-config` | Muestra la configuración que se cargará en el próximo reinicio. |
| `show interfaces FastEthernet0/0 include packets` | Filtra para mostrar solo las líneas relacionadas con paquetes transmitidos y recibidos. |
| `show interfaces FastEthernet0/0 include error` | Filtra para mostrar solo los errores de la interfaz. |


| Comando | Función |
|---------|---------|
| `enable` | Permite acceder al modo privilegiado, donde se pueden ejecutar comandos de administración avanzados. |
| `configure terminal` | Entra al modo de configuración global para realizar cambios en la configuración del dispositivo. |
| `interface <interfaz>` | Selecciona una interfaz física o subinterfaz específica para configurar (`<interfaz>` puede ser, por ejemplo, `FastEthernet0/0` o `GigabitEthernet0/1`). |
| `no shutdown` | Habilita o activa una interfaz que está deshabilitada por defecto, permitiendo que inicie la comunicación de red. |
| `encapsulation dot1Q <VLAN_ID>` | Configura una subinterfaz para manejar tráfico etiquetado de una VLAN específica usando el estándar 802.1Q. `<VLAN_ID>` es el identificador de la VLAN, como `100`, `102`, etc. |
| `ip address <IP> <máscara>` | Asigna una dirección IP a una interfaz o subinterfaz, con `<IP>` como la dirección IP (por ejemplo, `192.168.1.1`) y `<máscara>` como la máscara de subred (por ejemplo, `255.255.255.0`). |
| `encapsulation dot1Q <VLAN_ID> native` | Configura la subinterfaz para manejar el tráfico sin etiquetar, asignándolo a la VLAN nativa especificada (`<VLAN_ID>`). |
| `end` | Sale del modo de configuración y vuelve al modo privilegiado. |
| `copy running-config startup-config` | Guarda la configuración actual para que persista después de un reinicio del dispositivo. |

