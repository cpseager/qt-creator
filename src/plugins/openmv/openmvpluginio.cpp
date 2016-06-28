#include "openmvpluginio.h"

#define __USBDBG_CMD                0x30
#define __USBDBG_FW_VERSION         0x80
#define __USBDBG_FRAME_SIZE         0x81
#define __USBDBG_FRAME_DUMP         0x82
#define __USBDBG_FRAME_UPDATE       0x04
#define __USBDBG_SCRIPT_EXEC        0x05
#define __USBDBG_SCRIPT_STOP        0x06
#define __USBDBG_SCRIPT_SAVE        0x07 // ???
#define __USBDBG_SCRIPT_RUNNING     0x87
#define __USBDBG_TEMPLATE_SAVE      0x08
#define __USBDBG_DESCRIPTOR_SAVE    0x09
#define __USBDBG_ATTR_READ          0x8A
#define __USBDBG_ATTR_WRITE         0x0B
#define __USBDBG_SYS_RESET          0x0C
#define __USBDBG_SYS_BOOT           0x0D // ???
#define __USBDBG_JPEG_ENABLE        0x0E
#define __USBDBG_TX_BUF_LEN         0x8E
#define __USBDBG_TX_BUF             0x8F

#define __BOOTLDR_START             0xABCD0001
#define __BOOTLDR_RESET             0xABCD0002
#define __BOOTLDR_ERASE             0xABCD0004
#define __BOOTLDR_WRITE             0xABCD0008

#define FW_VERSION_RESPONSE_LEN     12
#define FRAME_SIZE_RESPONSE_LEN     12
#define SCRIPT_RUNNING_RESPONSE_LEN 4
#define ATTR_READ_RESPONSE_LEN      1
#define TX_BUF_LEN_RESPONSE_LEN     4

#define BOOTLDR_START_RESPONSE_LEN  4

#define IS_JPG(bpp)                 ((bpp) >= 3)
#define IS_RGB(bpp)                 ((bpp) == 2)

static inline void serializeByte(QByteArray &buffer, char value) // LittleEndian
{
    buffer.append(reinterpret_cast<const char *>(&value), 1);
}

static inline void serializeWord(QByteArray &buffer, short value) // LittleEndian
{
    buffer.append(reinterpret_cast<const char *>(&value), 2);
}

static inline void serializeLong(QByteArray &buffer, long value) // LittleEndian
{
    buffer.append(reinterpret_cast<const char *>(&value), 4);
}

char deserializeByte(QByteArray &buffer) // LittleEndian
{
    char r;
    memcpy(&r, buffer.data(), 1);
    buffer = buffer.mid(1);
    return r;
}

short deserializeWord(QByteArray &buffer) // LittleEndian
{
    short r;
    memcpy(&r, buffer.data(), 2);
    buffer = buffer.mid(2);
    return r;
}

long deserializeLong(QByteArray &buffer) // LittleEndian
{
    long r;
    memcpy(&r, buffer.data(), 4);
    buffer = buffer.mid(4);
    return r;
}

OpenMVPluginIO::OpenMVPluginIO(QObject *parent) : QObject(parent)
{
    m_port = nullptr;
    m_disablePrint = false;
    m_disableFrameBuffer = false;
    m_bootloaderMode = false;
}

QSerialPort *OpenMVPluginIO::serialPort() const
{
    return m_port;
}

bool OpenMVPluginIO::getDisablePrint() const
{
    return m_disablePrint;
}

bool OpenMVPluginIO::getDisableFrameBuffer() const
{
    return m_disableFrameBuffer;
}

bool OpenMVPluginIO::getBootloaderMode() const
{
    return m_bootloaderMode;
}

void OpenMVPluginIO::setSerialPort(QSerialPort *port)
{
    if(m_port)
    {
        m_receivedBytes.clear();
        m_commandQueue.clear();
        m_expectedHeaderQueue.clear();
        m_expectedDataQueue.clear();
    }

    QSerialPort *old = m_port;
    m_port = port;

    emit serialPortChanged(old, port);
}

void OpenMVPluginIO::processEvents()
{
    if(m_port)
    {
        m_receivedBytes.append(m_port->readAll());

        if((!m_expectedHeaderQueue.isEmpty())
        && (m_receivedBytes.size() >= m_expectedDataQueue.head()))
        {
            int receivedBytes = m_expectedDataQueue.dequeue();
            switch(m_expectedHeaderQueue.dequeue())
            {
                case __USBDBG_FW_VERSION:
                {
                    emit firmwareVersion(
                        deserializeLong(m_receivedBytes),
                        deserializeLong(m_receivedBytes),
                        deserializeLong(m_receivedBytes));
                    break;
                }
                case __USBDBG_FRAME_SIZE:
                {
                    m_frameSizeW = deserializeLong(m_receivedBytes);
                    m_frameSizeH = deserializeLong(m_receivedBytes);
                    m_frameSizeBPP = deserializeLong(m_receivedBytes);
                    if(m_frameSizeW && m_frameSizeH && m_frameSizeBPP)
                    {
                        int size = IS_JPG(m_frameSizeBPP)
                            ? m_frameSizeBPP
                            : (m_frameSizeW * m_frameSizeH * m_frameSizeBPP);

                        QByteArray buffer;
                        serializeByte(buffer, __USBDBG_CMD);
                        serializeByte(buffer, __USBDBG_FRAME_DUMP);
                        serializeLong(buffer, size);
                        m_commandQueue.enqueue(buffer);

                        m_expectedHeaderQueue.enqueue(__USBDBG_FRAME_DUMP);
                        m_expectedDataQueue.enqueue(size);
                    }
                    break;
                }
                case __USBDBG_FRAME_DUMP:
                {
                    emit frameBufferData(QPixmap::fromImage(IS_JPG(m_frameSizeBPP)
                    ? QImage::fromData(m_receivedBytes.left(receivedBytes), "JPG")
                    : QImage(reinterpret_cast<uchar *>(m_receivedBytes.left(receivedBytes).data()),
                        m_frameSizeW, m_frameSizeH, m_frameSizeW * m_frameSizeBPP,
                        IS_RGB(m_frameSizeBPP) ? QImage::Format_RGB16 : QImage::Format_Grayscale8)));
                    m_receivedBytes = m_receivedBytes.mid(receivedBytes);
                    break;
                }
                case __USBDBG_SCRIPT_RUNNING:
                {
                    emit scriptRunning(
                        deserializeLong(m_receivedBytes));
                    break;
                }
                case __USBDBG_ATTR_READ:
                {
                    emit scriptRunning(
                        deserializeByte(m_receivedBytes));
                    break;
                }
                case __USBDBG_TX_BUF_LEN:
                {
                    int len = deserializeLong(m_receivedBytes);
                    if(len)
                    {
                        QByteArray buffer;
                        serializeByte(buffer, __USBDBG_CMD);
                        serializeByte(buffer, __USBDBG_TX_BUF);
                        serializeLong(buffer, len);
                        m_commandQueue.enqueue(buffer);

                        m_expectedHeaderQueue.enqueue(__USBDBG_TX_BUF);
                        m_expectedDataQueue.enqueue(len);
                    }
                    break;
                }
                case __USBDBG_TX_BUF:
                {
                    emit printData(m_receivedBytes.left(receivedBytes));
                    m_receivedBytes = m_receivedBytes.mid(receivedBytes);
                    break;
                }
                case __BOOTLDR_START:
                {
                    emit gotBootloaderStart(
                        deserializeLong(m_receivedBytes) == static_cast<long>(__BOOTLDR_START));
                    break;
                }
            }
        }

        if((!m_disableFrameBuffer)
        && (!m_expectedHeaderQueue.contains(__USBDBG_FRAME_SIZE))
        && (!m_expectedHeaderQueue.contains(__USBDBG_FRAME_DUMP)))
        // Only execute this command if its not already being executed...
        {
            QByteArray buffer;
            serializeByte(buffer, __USBDBG_CMD);
            serializeByte(buffer, __USBDBG_FRAME_SIZE);
            serializeLong(buffer, FRAME_SIZE_RESPONSE_LEN);
            m_commandQueue.enqueue(buffer);

            m_expectedHeaderQueue.enqueue(__USBDBG_FRAME_SIZE);
            m_expectedDataQueue.enqueue(FRAME_SIZE_RESPONSE_LEN);
        }

        if((!m_disablePrint)
        && (!m_expectedHeaderQueue.contains(__USBDBG_TX_BUF_LEN))
        && (!m_expectedHeaderQueue.contains(__USBDBG_TX_BUF)))
        // Only execute this command if its not already being executed...
        {
            QByteArray buffer;
            serializeByte(buffer, __USBDBG_CMD);
            serializeByte(buffer, __USBDBG_TX_BUF_LEN);
            serializeLong(buffer, TX_BUF_LEN_RESPONSE_LEN);
            m_commandQueue.enqueue(buffer);

            m_expectedHeaderQueue.enqueue(__USBDBG_TX_BUF_LEN);
            m_expectedDataQueue.enqueue(TX_BUF_LEN_RESPONSE_LEN);
        }

        if(!m_commandQueue.isEmpty())
        {
            QByteArray command = m_commandQueue.dequeue();

            if(m_port->write(command) != command.size())
            {
                setSerialPort(nullptr); // shutdown
            }
        }
    }
}

void OpenMVPluginIO::getFirmwareVersion()
{
    QByteArray buffer;
    serializeByte(buffer, __USBDBG_CMD);
    serializeByte(buffer, __USBDBG_FW_VERSION);
    serializeLong(buffer, FW_VERSION_RESPONSE_LEN);
    m_commandQueue.enqueue(buffer);

    m_expectedHeaderQueue.enqueue(__USBDBG_FW_VERSION);
    m_expectedDataQueue.enqueue(FW_VERSION_RESPONSE_LEN);
}

void OpenMVPluginIO::frameUpdate()
{
    QByteArray buffer;
    serializeByte(buffer, __USBDBG_CMD);
    serializeByte(buffer, __USBDBG_FRAME_UPDATE);
    serializeLong(buffer, 0);
    m_commandQueue.enqueue(buffer);
}

void OpenMVPluginIO::scriptExec(const QByteArray &data)
{
    QByteArray buffer;
    serializeByte(buffer, __USBDBG_CMD);
    serializeByte(buffer, __USBDBG_SCRIPT_EXEC);
    serializeLong(buffer, data.size());
    buffer.append(data);
    m_commandQueue.enqueue(buffer);
}

void OpenMVPluginIO::scriptStop()
{
    QByteArray buffer;
    serializeByte(buffer, __USBDBG_CMD);
    serializeByte(buffer, __USBDBG_SCRIPT_STOP);
    serializeLong(buffer, 0);
    m_commandQueue.enqueue(buffer);
}

void OpenMVPluginIO::getScriptRunning()
{
    QByteArray buffer;
    serializeByte(buffer, __USBDBG_CMD);
    serializeByte(buffer, __USBDBG_SCRIPT_RUNNING);
    serializeLong(buffer, SCRIPT_RUNNING_RESPONSE_LEN);
    m_commandQueue.enqueue(buffer);

    m_expectedHeaderQueue.enqueue(__USBDBG_SCRIPT_RUNNING);
    m_expectedDataQueue.enqueue(SCRIPT_RUNNING_RESPONSE_LEN);
}

void OpenMVPluginIO::templateSave(long x, long y, long w, long h, const QByteArray &path)
{
    QByteArray buffer;
    serializeByte(buffer, __USBDBG_CMD);
    serializeByte(buffer, __USBDBG_TEMPLATE_SAVE);
    serializeLong(buffer, sizeof(x) + sizeof(y) + sizeof(w) + sizeof(h) + path.size());
    serializeLong(buffer, x);
    serializeLong(buffer, y);
    serializeLong(buffer, w);
    serializeLong(buffer, h);
    buffer.append(path);
    m_commandQueue.enqueue(buffer);
}

void OpenMVPluginIO::descriptorSave(short x, short y, short w, short h, const QByteArray &path)
{
    QByteArray buffer;
    serializeByte(buffer, __USBDBG_CMD);
    serializeByte(buffer, __USBDBG_DESCRIPTOR_SAVE);
    serializeLong(buffer, sizeof(x) + sizeof(y) + sizeof(w) + sizeof(h) + path.size());
    serializeWord(buffer, x);
    serializeWord(buffer, y);
    serializeWord(buffer, w);
    serializeWord(buffer, h);
    buffer.append(path);
    m_commandQueue.enqueue(buffer);
}

void OpenMVPluginIO::set_attr(short attribute, short value)
{
    QByteArray buffer;
    serializeByte(buffer, __USBDBG_CMD);
    serializeByte(buffer, __USBDBG_ATTR_WRITE);
    serializeLong(buffer, 0);
    serializeWord(buffer, attribute);
    serializeWord(buffer, value);
    m_commandQueue.enqueue(buffer);
}

void OpenMVPluginIO::getAttribute(short attribute)
{
    QByteArray buffer;
    serializeByte(buffer, __USBDBG_CMD);
    serializeByte(buffer, __USBDBG_ATTR_READ);
    serializeLong(buffer, ATTR_READ_RESPONSE_LEN);
    serializeWord(buffer, attribute);
    m_commandQueue.enqueue(buffer);

    m_expectedHeaderQueue.enqueue(__USBDBG_ATTR_READ);
    m_expectedDataQueue.enqueue(ATTR_READ_RESPONSE_LEN);
}

void OpenMVPluginIO::sysReset()
{
    QByteArray buffer;
    serializeByte(buffer, __USBDBG_CMD);
    serializeByte(buffer, __USBDBG_SYS_RESET);
    serializeLong(buffer, 0);
    m_commandQueue.enqueue(buffer);
}

void OpenMVPluginIO::jpegEnable(bool enabled)
{
    QByteArray buffer;
    serializeByte(buffer, __USBDBG_CMD);
    serializeByte(buffer, __USBDBG_JPEG_ENABLE);
    serializeLong(buffer, 0);
    serializeWord(buffer, enabled ? true : false);
    m_commandQueue.enqueue(buffer);
}

void OpenMVPluginIO::disablePrint(bool enable)
{
    m_disablePrint = enable;
}

void OpenMVPluginIO::disableFrameBuffer(bool enable)
{
    m_disableFrameBuffer = enable;
}

void OpenMVPluginIO::bootloaderMode(bool enable)
{
    m_bootloaderMode = enable;
}

void OpenMVPluginIO::bootloaderStart()
{
    QByteArray buffer;
    serializeLong(buffer, __BOOTLDR_START);
    m_commandQueue.enqueue(buffer);

    m_expectedHeaderQueue.enqueue(__BOOTLDR_START);
    m_expectedDataQueue.enqueue(BOOTLDR_START_RESPONSE_LEN);
}

void OpenMVPluginIO::bootloaderReset()
{
    QByteArray buffer;
    serializeLong(buffer, __BOOTLDR_RESET);
    m_commandQueue.enqueue(buffer);
}

void OpenMVPluginIO::flashErase(long sector)
{
    QByteArray buffer;
    serializeLong(buffer, __BOOTLDR_ERASE);
    serializeLong(buffer, sector);
    m_commandQueue.enqueue(buffer);
}

void OpenMVPluginIO::flashWrite(const QByteArray &data)
{
    QByteArray buffer;
    serializeLong(buffer, __BOOTLDR_WRITE);
    buffer.append(data);
    m_commandQueue.enqueue(buffer);
}