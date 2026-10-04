#ifndef LOGGER_HPP
#define LOGGER_HPP

/**
 * @brief Logger global simples que grava mensagens em arquivo.
 *
 * Interface estática: chame init() uma vez na inicialização para abrir o
 * arquivo de log e shutdown() no encerramento. Em geral não se chama log()
 * diretamente — use as macros LOG_INFO/LOG_WARN/LOG_ERROR de log_macros.hpp,
 * que preenchem classe e método automaticamente.
 *
 * @see log_macros.hpp
 */
class Logger {
  public:
    /**
     * @brief Inicializa o logger abrindo o arquivo de saída.
     * @param filename Caminho do arquivo de log.
     */
    static void init(const char* filename);

    /** @brief Fecha o arquivo de log e libera recursos. */
    static void shutdown();

    /**
     * @brief Registra uma mensagem com origem (classe/método).
     * @param className  Nome da classe de origem.
     * @param methodName Nome do método de origem.
     * @param message    Texto da mensagem.
     */
    static void log(const char* className, const char* methodName, const char* message);
};

#endif
