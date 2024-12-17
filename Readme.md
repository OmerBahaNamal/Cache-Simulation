# Projektbericht

Die Aufgabe dieses Projekts war es eine Cache-Simulation eines sowohl direkt abbildenden- als auch eines vierfach assoziativen Caches zu erstellen und deren Differenzen zu untersuchen.

Wir begannen die Bearbeitung mit einer Literaturrecherche um primär übliche Größen der Caches, sowie die Latenzen bei Zugriffen auf jeweils diese und den Hauptspeicher in Erfahrung zu bringen. So ergaben sich 32KB als typische Größe für beide Caches[^1][^2], sowie eine Latenz von 1ns für L1-Caches und etwa das 200-fache für Hauptspeicherzugriffe[^3].

Im Verlauf unserer Recherche wurde uns bewusst, dass heutzutage fast ausschließlich Set-assoziative Caches verwendet werden. Dabei sind direkt abbildende Caches einfacher zu implementieren, haben einen geringeren Energieverbrauch und theoretisch auch schnellere Zugriffe. Allerdings entstehen durch den Mangel einer Logik zum Ersetzen der Cachezeilen deutlich mehr Conflict-Misses. Im Vergleich dazu hat ein vierfach assoziativer Cache diese, unterliegt dadurch aber in den genannten Aspekten. Allerdings kommt es zu deutlich weniger Conflict-Misses, wodurch die Performanz und Effizienz insgesamt stark gesteigert wird. Die erhöhte Komplexität und der Energieverbrauch werden in modernen Prozessoren in Kauf genommen.[^4]

Diese Rechercheergebnisse wurden durch intensives Testen bestätigt. Bei sinnvollen Anfragen, simuliert der vierfach assoziative Cache tatsächlich weniger misses als der Direkt abbildende Cache. Bei sehr wenigen oder komplett zufälligen Anfragen kann dieser aber auch schneller sein.

## persönlichen Beiträge

Alptuğ:
Ich habe mich hauptsächlich mit dem Rahmenprogramm und dem Parsing der Eingabedatei beschäftigt. Der wichtigste Teil des Rahmenprogramms ist das Parsing in das richtige Format und das Validieren der Requests. In unserer Implementierung ist eine Request valide, wenn sie nur aus Whitespaces und/oder erlaubten Zeichen besteht.

Tim Albrecht:
Ich habe mich hauptsächlich mit der Simulation des direkt abbildenden Caches befasst. Dabei war vor allem das Erstellen eines CPU-Moduls ein wichtiger Grundstein um die Anfragen jeweils zur richtigen Zeit zu bearbeiten und die Abstraktion des Programms zu erhöhen. Den Cache habe ich ebenso wie den Hauptspeicher als eine Map dargestellt, da so einfach Werte initialisiert und nicht immer der ganze Speicher reserviert werden muss.

Ömer:
Ich habe mich hauptsächlich mit dem vierfach assoziativen Cache beschäftigt. Zum Beginn der Implementierung, haben wir uns die Frage gestellt, welcher Ersetzungsalgorithmus verwendet werden sollte. Die Recherche ergab, dass häufigsten Ersetzungsalgorithmen FIFO, LIFO, LRU, LFU sind. Wir haben uns für FIFO entschieden, da die Implementierung einfacher ist. Wir haben die Logik mithilfe der Deque aus der Standard-Bibliothek realisiert. Jedes Set hat eine Deque. Ein neuer Tag wird mit pushBack() an der letzten Stelle der Deque hinzugefügt. Wenn das Set voll ist, wird das erste und somit älteste Element mit popFront() aus dem Cache gelöscht.

[^1]: 7-Zip; [LZMA Benchmark](https://www.7-cpu.com)
[^2]: PassMark Software; [CPU Benchmarks](https://www.cpubenchmark.net)
[^3]: Intel; [Memory Performance in a Nutshell](https://www.intel.com/content/www/us/en/developer/articles/technical/memory-performance-in-a-nutshell.html)
[^4]: IN0004 Einführung in die Rechnerarchitektur
