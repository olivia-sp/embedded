/**
 * \file Ifx_Fifo.h
 * \brief FIFO buffer functions
 * \ingroup IfxLld_lib_datahandling_fifo
 *
 * \version iLLD_1_0_1_12_0_1
 * \copyright Copyright (c) 2013 Infineon Technologies AG. All rights reserved.
 *
 *
 *                                 IMPORTANT NOTICE
 *
 * Use of this file is subject to the terms of use agreed between (i) you or
 * the company in which ordinary course of business you are acting and (ii)
 * Infineon Technologies AG or its licensees. If and as long as no such terms
 * of use are agreed, use of this file is subject to following:
 *
 * Boost Software License - Version 1.0 - August 17th, 2003
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer, must
 * be included in all copies of the Software, in whole or in part, and all
 * derivative works of the Software, unless such copies or derivative works are
 * solely in the form of machine-executable object code generated by a source
 * language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * \defgroup IfxLld_lib_datahandling_fifo FIFO
 * This module implements the FIFO buffer functionality.
 * \ingroup IfxLld_lib_datahandling
 *
 */

#ifndef IFX_FIFO_H
#define IFX_FIFO_H 1
//------------------------------------------------------------------------------
#include "Ifx_Cfg.h"
#include "Cpu/Std/IfxCpu_Intrinsics.h"
//------------------------------------------------------------------------------

/** Shared data of the FIFO
 *
 */
typedef struct
{
    Ifx_SizeT count;            /**< \brief number of bytes contained in the buffer */
    sint32    readerWaitx;      /**< \brief Number of bytes that the reader is waiting for. When the writer modify it to 0 the reader get signaled */
    sint32    writerWaitx;      /**< \brief Number of byte that the writer expect to be free. When the reader modify it to 0 the reader get signaled */
    Ifx_SizeT maxcount;         /**< \brief Highest value seen in the count */
} Ifx_Fifo_Shared;

/** \addtogroup IfxLld_lib_datahandling_fifo
 * \{ */
/** Fifo object
 *
 */
typedef struct _Fifo
{
    void            *buffer;                /**< \brief aligned on 64 bit boundary */
    Ifx_Fifo_Shared  shared;                /**< \brief  data shared between reader / writer */
    Ifx_SizeT        startIndex;            /**< \brief buffer valid data start index */
    Ifx_SizeT        endIndex;              /**< \brief buffer valid data end index */
    Ifx_SizeT        size;                  /**< \brief multiple of 8 bit, max 0xFFF8 */
    Ifx_SizeT        elementSize;           /**< \brief minimum number of bytes (block) added / removed to / from the buffer */
    volatile boolean eventReader;           /**< \brief event set by the writer to signal the reader that the required data are available in the buffer */
    volatile boolean eventWriter;           /**< \brief event set by the reader to signal the writer that the required free space are available in the buffer */
} Ifx_Fifo;

/** \brief Indicates if the required number of bytes are available in the buffer
 *
 * Should not be called from an interrupt as this function may wait forever
 * \param fifo Pointer on the Fifo object
 * \param count in bytes
 * \param timeout in system timer ticks
 *
 * \return TRUE if at least count bytes can be read from the buffer, else
 * the Event is armed to be set when the buffer count is bigger or equal to the requested count
 */
IFX_EXTERN boolean Ifx_Fifo_canReadCount(Ifx_Fifo *fifo, Ifx_SizeT count, Ifx_TickTime timeout);
/** \brief  Indicates if there is enough free space to write the data in the buffer
 *
 * Should not be called from an interrupt as this function may wait forever
 *
 * \param fifo Pointer on the Fifo object
 * \param count in bytes
 * \param timeout in system timer ticks
 *
 * \return TRUE if at least count bytes can be written to the buffer,
 * if not the Event is armed to be set when the buffer free count is bigger or equal to the requested count
 */
IFX_EXTERN boolean Ifx_Fifo_canWriteCount(Ifx_Fifo *fifo, Ifx_SizeT count, Ifx_TickTime timeout);

/** \brief Clear fifo contents.
 *
 * \param fifo Pointer on the Fifo object
 *
 * \return void
 */
IFX_EXTERN void Ifx_Fifo_clear(Ifx_Fifo *fifo);

/** \brief Create a Fifo object
 *
 * The memory required for the object is allocated dynamically.
 *
 * \param size Specifies the FIFO buffer size in bytes
 * \param elementSize Specifies data element size in bytes. size must be bigger or equal to elemenntSize.
 *
 * \return returns a pointer to the FIFO object
 *
 * \see Ifx_Fifo_destroy()
 */
IFX_EXTERN Ifx_Fifo *Ifx_Fifo_create(Ifx_SizeT size, Ifx_SizeT elementSize);

/** \brief Destroy the FIFO object
 *
 * This function must be called to destroy the fifo object when created with \ref Ifx_Fifo_create()
 *
 * \param fifo Pointer on the Fifo object
 * \return void
 *
 * \see   Ifx_Fifo_create()
 */
IFX_EXTERN void Ifx_Fifo_destroy(Ifx_Fifo *fifo);

/** \brief Initialize the FIFO buffer object
 *
 * \param buffer Specifies the FIFO object address.
 * \param size Specifies the FIFO buffer size in bytes
 * \param elementSize Specifies data element size in bytes. size must be bigger or equal to elemenntSize.
 *
 * \return Returns a pointer on the FIFO object
 *
 * \note: The buffer parameter must point on a free memory location where the
 * buffer object will be initialised. The size of this area must be at least
 * equals to "size + sizeof(Ifx_Fifo) + 8". Not taking this in account may result
 * in unpredictable behavior.
 */
IFX_EXTERN Ifx_Fifo *Ifx_Fifo_init(void *buffer, Ifx_SizeT size, Ifx_SizeT elementSize);

/** \brief Read data from a fifo and remove them from the buffer.
 *
 * Only complete elements are returned, if count is not a multiple of
 * elementSize then the incomplete element is not read/removed from the buffer.
 *
 * \param fifo Pointer on the Fifo object
 * \param data Pointer to the data buffer for storing values
 * \param count in bytes
 * \param timeout in system timer ticks
 *
 * \return return the number of byte that could not be read
 */
IFX_EXTERN Ifx_SizeT Ifx_Fifo_read(Ifx_Fifo *fifo, void *data, Ifx_SizeT count, Ifx_TickTime timeout);

/** \brief Write data into a fifo.
 *
 * Only complete elements are written to the buffer, if count is not a multiple of
 * elementSize then the incomplete element are not written to the buffer.
 *
 * \param fifo Pointer on the Fifo object
 * \param data Pointer to the data buffer to write into the Fifo
 * \param count in bytes
 * \param timeout in system timer ticks
 */
IFX_EXTERN Ifx_SizeT Ifx_Fifo_write(Ifx_Fifo *fifo, const void *data, Ifx_SizeT count, Ifx_TickTime timeout);

/** \brief Empty the fifo
 *
 * \param fifo Pointer on the Fifo object
 * \param timeout in system timer ticks
 *
 * \return TRUE if the buffer is emptied.
 */
IFX_INLINE boolean Ifx_Fifo_flush(Ifx_Fifo *fifo, Ifx_TickTime timeout)
{
    return Ifx_Fifo_canWriteCount(fifo, fifo->size, timeout);
}


/**
 * \brief Returns the size of the data in the buffer in bytes
 *
 * \param fifo Pointer on the Fifo object
 *
 * Note as the Ifx_Fifo_write / Ifx_Fifo_read function does only write blocks which are
 * a multiple of fifo->elementSize, the Ifx_Fifo_readCount / Ifx_Fifo_writeCount return
 * a multiple of fifo->elementSize
 *
 * \return Returns the size of the data in the buffer in bytes
 */
IFX_INLINE Ifx_SizeT Ifx_Fifo_readCount(Ifx_Fifo *fifo)
{
    return fifo->shared.count;
}


/** \brief Returns the free size in bytes
 *
 * \param fifo Pointer on the Fifo object
 *
 * Note as the Ifx_Fifo_write / Ifx_Fifo_read function does only write blocks which are
 * a multiple of fifo->elementSize, the Ifx_Fifo_readCount / Ifx_Fifo_writeCount return
 * a multiple of fifo->elementSize
 *
 * \return Returns the free size in bytes
 */
IFX_INLINE Ifx_SizeT Ifx_Fifo_writeCount(Ifx_Fifo *fifo)
{
    return (Ifx_SizeT)(fifo->size - Ifx_Fifo_readCount(fifo));
}


/** \brief Indicates if the fifo is empty
 *
 * \param fifo Pointer on the Ifx_Fifo object
 *
 * \retval TRUE is the buffer is empty
 * \retval FALSE is the buffer is not empty
 */
IFX_INLINE boolean Ifx_Fifo_isEmpty(Ifx_Fifo *fifo)
{
    return (Ifx_Fifo_readCount(fifo) != FALSE) ? FALSE : TRUE;
}


/**\}*/
//------------------------------------------------------------------------------
#endif
