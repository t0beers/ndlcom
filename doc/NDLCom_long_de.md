![ndlcom_logo](ndlcom_logo.png)

# NDLCom

Moderne robotische Systeme verfügen über zunehmend mehr dezentrale Verarbeitungsknoten. Diese werden zum Beispiel verwendet um im System verteilte Sensoren auszuwerten oder um eine hochfrequente Regelung von Motoren zu ermöglichen. Das Zusammenfassen der Knoten in einem paketbasierten Punkt-zu-Punkt Netzwerk erlaubt einen flexiblen Datenaustausch zwischen beliebigen Teilnehmern. NDLCom (Node Data Link Communication) definiert ein einfaches Paketformat für Datenaustausch mittels serieller Schnittstellen, das im Gegensatz zu traditionellen Netzwerktechnologien wie IP einen relativ geringen Ressourcenbedarf hat. Das erlaubt auch Mikrocontrollern oder FPGAs mit eingeschränkter Rechenleistung die Kommunikation mit anderen Knoten.

Zur Segmentierung des Datenstroms werden ähnlich wie bei [HDLC](https://tools.ietf.org/html/rfc1662) definierte Marker verwendet. Der Header eines Pakets besteht aus 4 Byte: Jeweils einer Empfänger- und Absenderadresse für bis zu 254 verschiedenen Adressen sowie der speziellen "Broadcast"-Adresse. Weiterhin enthalten ist ein Paket-Zähler zur Erkennung von verlorenen Paketen. Der Nutzlast von maximal 255 Bytes ist die entsprechende Längenangabe vorangestellt. Jedes Paket ist am Ende mit einer 16Bit [Prüfsumme](crc.md) versehen um Übertragungsfehler zu erkennen. Das Wiederherstellen von verloren gegangenen oder beschädigten Paketen wird vom Protokoll nicht unterstützt. Jeder Teilnehmer muss in der Netzwerktopologie über einen eindeutigen Pfad zu erreichen sein, Schleifen sind nicht erlaubt.

![protocol](protocol.png)

Es existiert eine mit Qt4 geschriebene graphische Benutzerumgebung für Linux, um mit einem normalen Computer in einem NDLCom-Netzwerk zu agieren. Es können mehrere Verbindungen über USB, TCP und UDP gleichzeitig aufgebaut werden. Verschiedene Komponenten erlauben Pakete zu empfangen oder zu versenden. Ebenso können die enthaltenen Nutzdaten in ihrem zeitlichen Verlauf ausgewertet und binär oder als CSV zur weiteren Verwendung gespeichert werden. Eine Reihe spezialisierter Widgets ermöglicht die bequeme Bedienung von Teilfunktionen einzelner Knoten. Es bestehen Implementierungen von NDLCom in C/C++ und VHDL, verwendet werden zur Zeit Spartan3/6, STM32 und AVR.

![CommonGUI](CommonGUI_NDLComDemo.png)
