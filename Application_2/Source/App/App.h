#ifndef APP_H
#define APP_H

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Starts the application runtime.
     *
     * Confirms the current boot image in metadata when applicable, then enters
     * the main application loop.
     */
    void App_Start();

#ifdef __cplusplus
}
#endif

#endif /* APP_H */
