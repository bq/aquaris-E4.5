
#include "typedefs.h"
#include "platform.h"
#include "meta.h"
#include "uart.h"

#if CFG_UART_TOOL_HANDSHAKE

/*============================================================================*/
/* CONSTAND DEFINITIONS                                                       */
/*============================================================================*/
#define MOD                 "[TOOL]"
#define UART_SYNC_TIME       (150)    /* in ms */

/*============================================================================*/
/* GLOBAL VARIABLES                                                           */
/*============================================================================*/
static ulong g_meta_ready_start_time = 0;

/*============================================================================*/
/* INTERNAL FUNCTIONS                                                         */
/*============================================================================*/
static bool uart_listen(struct bldr_comport *comport, uint8 *data, uint32 size, int retry, uint32 tmo_ms)
{
    u8 c = 0;
    int rx_cnt = 0;
    int tmo_en = (tmo_ms) ? 1 : 0;
    ulong start_time;

    for (; retry > 0; retry--) {
        start_time = get_timer(0);
        while(1) {
            if (tmo_en && (get_timer(start_time) > tmo_ms))
                break;

            /* kick watchdog to avoid cpu reset */
            if (!tmo_en)
                platform_wdt_kick();

            GetUARTBytes(&c, 1, 10);
            if (c != 0) {
                *data++ = (uint8)c;
                rx_cnt++;
            }

            if (rx_cnt == size){
                print("%s <UART> listen  success!\n",MOD);
                return TRUE;
            }
        }
    }

      print("%s <UART> listen  ended, receive size:%d!\n",MOD,rx_cnt);

    return FALSE;
}

static int uart_send(u8 *buf, u32 len)
{
    if (CFG_UART_META != CFG_UART_LOG)
        mtk_serial_set_current_uart(CFG_UART_META);

    while (len--) {
        PutUARTByte(*buf++);
    }

    if (CFG_UART_META != CFG_UART_LOG)
        mtk_serial_set_current_uart(CFG_UART_LOG);

    return 0;
}

static int uart_recv(u8 *buf, u32 size, u32 tmo_ms)
{
    int ret;
    
    if (CFG_UART_META != CFG_UART_LOG)
        mtk_serial_set_current_uart(CFG_UART_META);

    ret = GetUARTBytes(buf, size, tmo_ms);
    
    if (CFG_UART_META != CFG_UART_LOG)
        mtk_serial_set_current_uart(CFG_UART_LOG);

    return ret;
}

static bool uart_handshake_handler(struct bldr_command_handler *handler)
{
    bool result = TRUE;
    bool avail;
    int  sync_time;
    uint8 buf[HSHK_TOKEN_SZ + 1] = {'\0'};
    struct bldr_command cmd;
    struct bldr_comport comport;
    struct comport_ops uart_ops = {.send = uart_send, .recv = uart_recv};


    if (g_boot_mode == META_BOOT || g_boot_mode == ADVMETA_BOOT) {
        print("%s aleady detected meta tool!\n", MOD);
        result =FALSE;
        goto exit;
    }
    

    comport.type = COM_UART;
    comport.tmo  = UART_SYNC_TIME;
    comport.ops  = &uart_ops;

    sync_time = UART_SYNC_TIME - get_timer(g_meta_ready_start_time);
    sync_time = sync_time <= 0 ? 5 : sync_time;
    sync_time = sync_time > UART_SYNC_TIME ? UART_SYNC_TIME : sync_time;

    /* detect tool existence */
    mtk_serial_set_current_uart(CFG_UART_META);
    avail = uart_listen(&comport, buf, HSHK_TOKEN_SZ, 1, sync_time);
    mtk_serial_set_current_uart(CFG_UART_LOG);

    if (!avail) {
        result = FALSE;
        goto exit;
    }

    cmd.data = buf;
    cmd.len  = HSHK_TOKEN_SZ;
    
    result = handler->cb(handler, &cmd, &comport);

exit:

    if (CFG_UART_META == CFG_UART_LOG) {
        /* enable log message again since no tool is connected */
        if (result != TRUE) {
            /* init to log baudrate */
            mtk_uart_init(UART_SRC_CLK_FRQ, CFG_LOG_BAUDRATE);
            /* enable log and flush the log buffer if log message available */
            log_ctrl(1);
        }
    }
    print("\n");
    print("%s <UART> wait sync time %dms->%dms\n", MOD, UART_SYNC_TIME, sync_time);
    print("%s <UART> receieved data: (%s)\n", MOD, buf);

    return result;
}

/*============================================================================*/
/* GLOBAL FUNCTIONS                                                           */
/*============================================================================*/
void uart_handshake_init(void)
{
    /* switch to meta port */
    mtk_serial_set_current_uart(CFG_UART_META);

    /* if meta and log ports are SAME, need to re-init meta port with 
     * different baudrate and disable log output during handshake to avoid
     * influence.
     */
    if (CFG_UART_META == CFG_UART_LOG) {
        /* to prevent sync error with PC */
	    gpt_busy_wait_us(160);
	    /* init to meta baudrate */
        mtk_uart_init(UART_SRC_CLK_FRQ, CFG_META_BAUDRATE);
        /* disable log so that log message will be kept in log buffer */
        log_buf_ctrl(0);
        log_ctrl(0);
    }

    /* send meta ready to tool via meta port first then let meta port to listen 
     * meta response in the background to reduce the handshake wait time later.
     */    
    uart_send((u8*)HSHK_COM_READY, strlen(HSHK_COM_READY));
    mtk_serial_set_current_uart(CFG_UART_LOG);
    
    g_meta_ready_start_time = get_timer(0);
}

bool uart_handshake(struct bldr_command_handler *handler)
{
    bool result = FALSE;


    result = uart_handshake_handler(handler);


    if (result == TRUE) {
        print("%s <UART> handshake pass!\n",MOD);
    }
    return result;
}

#endif /* CFG_UART_TOOL_HANDSHAKE */


