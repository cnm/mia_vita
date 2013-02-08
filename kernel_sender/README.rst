File structure
==============

This is the file structure::

  .
  ├── compile.bash                - Use this to compile (because of dependencies in modules)
  ├── miavita_packet.h            - It is here that the MIAVITA message structure is defined
  ├── sender_kthread.c            - Module which sends packets and times them
  └── user                        - Userland program to receive the MIA Vita packets and to write them to two file (json and raw)
      ├── ...
      ...

Objective
=========

    Kernel sender is a module application which reads data from the buffer created at interruption/proc_entry.c and sends it to the other nodes.

Drawing
=======

Logic behind the synchronization::


 +---------+                                             +---------+                         +---------+
 |   App   |  (1)        (Kernel Module)                 |   App   |                     (1) |   App   |
 |---------|   |                                         |---------|                      ↑  |---------|
 |  Trans  |   |         (UDP)                           |  Trans  |                      |  |  Trans  |
 |---------|   |                                         |---------|                      |  |---------|
 |  Inter  |   |         (IP)                            |  Inter  |                      |  |  Inter  |
 |---------|   |                                         |---------|                      |  |---------|
 |         |   ↓                                       ↔ |         | ↔                    |  |         |
 |  Link   |  (1->2)     (MAC - Kernel Module)    (2->1) |  Link   | (1->2)           (2->1) |  Link   |
 |         |   --------------------------------------->  |         | --------------------->  |         |
 +---------+                                             +---------+                         +---------+

 1    )  Happens at the interruption at proc_entry.c  (function: read_four_channels in interruption/fpga.c)
 1-2  )  Happens in the rt2501 module                 (function: synch_out_data_packet in rt2501/sources/Module/sync_proto.c)
 2-1  )  Happens at the rt2501 module                 (function: synch_in_data_packet  in rt2501/sources/Module/sync_proto.c)


To translate
------------

A directoria kernel_sender contém o código que prepara os pacotes e envia-os para o nó sink. Este por sua vez deverá ter uma instância do programa que recebe os dados a correr. Este programa encontra-se em kernel\_sender/user.

O programa que envia os dados sender\_kthread.ko opera em kernel land. A principal razão para este facto prende-se com a forma como as amostras são recolhidas do ADC. Estas são recolhidas por um módulo de kernel int\_mod.ko localizado em interruption criado pelo João Trindade que preenche um buffer com tais amostras e as temporiza. Seria possível ler este buffer a partir de um programa em user land, contudo isto traria um overhead indesejável, pelo que optou-se por fazer o programa cliente também em kernel land.

    Como seria de esperar, o módulo que envia os dados depende do módulo que recolhe as amostras. Como tal, este último tem de ser compilado primeiro. Na directoria kernel\_sender encontra-se um script com o nome compile.bash. Este script trata de compilar tanto o módulo int\_mod.ko como o módulo sender\_kthread.ko, resolvendo todas as dependências.

    Quando o módulo sender\_kthread.ko foi desenvolvido, o protocol de sincronização estava em fase de testes. O objectivo dos testes era comparar o atraso dado por dispositivos GPS, face ao atraso medido pelo protocolo. Por esta razão, várias zonas do código encontram-se circunscritas por **#ifdef \_\_GPS\_\_ ... \#endif**. Para que o código seja compilado para os testes com GPS basta passar ao compilador a flag **-D\_\_GPS\_\_**. Como isto trata-se de um módulo de kernel, é preciso abrir a Makefile e adicionar a linha:

    \begin{center}
      {\tt EXTRA\_CFLAGS+=-D\_\_GPS\_\_}
    \end{center}

    Note-se que ao compilar este módulo para correr os testes com GPS, é necessário também compilar o código do driver (Secção \ref{sec:driver}) e o programa que recebe os dados com a mesma \emph{flag}.

    É extremamente importante perceber que esta flag não faz com que o valor de criação do pacote passe a ser medido pelo GPS. Apenas prepara o código para os testes com GPS. O valor de criação do pacote continua a ser medido pelo protocolo de sincronização. Para alterar o código de modo a usar o valor do GPS como tempo de criação do pacote é necessário alterar as zonas marcadas com comentários no código.
    (TODO Fred Marca com comentários e faz listagem dos ficheiros sff)

    É ainda possível compilar o módulo com informação de \emph{debug} que é emitida para */var/log/syslog}*. Neste caso é necessária a *flag* **-DDBG**.

    O módulo sender\_kthread.ko tem uma funcionalidade muito simples. Limita-se a criar uma thread, que por sua vez abre um socket e de X em X tempo vai lendo N amostras do buffer de amostras em int\_mod.ko (ficheiro source: proc\_entry.c). Por cada amostra é criado um pacote e os dados são enviados para o sink. É preciso especificar pelo menos o IP do sink e o do próprio nó. Isto pode ser feito em runtime. Para saber todos os parâmetros de um módulo basta usar o comando:

    \begin{flushleft}
      {\tt 
        modinfo sender\_kthread.ko\\\hfill\\
        filename:       ./sender\_kthread.ko\\
        description:    This module spawns a thread which reads the buffer exported by João and
                        sends samples accross the network.\\
        author:         Frederico Gonçalves, [frederico.lopes.goncalves@gmail.com]\\
        license:        GPL v2\\
        depends:        int\_mod\\
        vermagic:       2.6.24.4 mod\_unload ARMv4 \\
        parm:           bind\_ip:This is the ip which the kernel thread will bind to. Default is localhost. (charp)\\
        parm:           sink\_ip:This is the sink ip. Default is localhost. (charp)\\
        parm:           sport:This is the UDP port which the sender thread will bind to. Default is 57843. (ushort)\\
        parm:           sink\_port:This is the sink UDP port. Default is 57843. (ushort)\\
        parm:           node\_id:This is the identifier of the node running this thread. Defaults to 0. (ushort)\\
        parm:           read\_t:The sleep time for reading the buffer. (uint)
      }
    \end{flushleft}

    Especificar parametros para um módulo é bastante simples. Como exemplo:

    \begin{flushleft}
      {\tt insmod sender\_kthread.ko bind\_ip=''192.168.2.123'' sink\_ip=''192.168.2.1''}
    \end{flushleft}

    Por fim é preciso ter em conta a especificação do pacote de dados (Secção \ref{sec:packet_specification}). Todos os campos são enviados em network byte order, que é Big Endian. Os processadores ARM podem funcionar tanto em Little Endian, como em Big Endian. Infelizmente, os processadores das placas TS-7500 funcionam em Little Endian, pelo que os dados têm de ser convertidos antes de serem enviados. Para tipos de dados alinhados, isto é, inteiros de 16, 32 e 64 bits o kernel já fornece funções que fazem a conversão. Contudo, cada amostra tem 24 bits, pelo que não existe nenhuma função que faça a conversão por nós. Esta foi então implementada na função read\_nsamples localizada no ficheiro interruption/proc\_entry.c. O modo como foi implementada foi pensado para ser o mais rápido possível, evitando ciclos. Contudo é preciso ter especial cuidado com o seguinte. Esta conversão depende de dois grande factores:

1. Assume que o ARM trata os dados como Little Endian. Se por alguma razão o hardware mudar, é preciso verificar se esta conversão está a ser feita correctamente.
2. Assume que o código no ficheiro interruption/fpga.c lê as amostras de uma forma especifica. Se este código mudar, é necessário verificar se a conversão contínua a ser bem feita. Por outras palavras, o código da função read\_nsamples é extremamente dependente do código do ficheiro interruption/fpga.c.
