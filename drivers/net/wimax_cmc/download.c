/*
 * download.c
 *
 * Firmware download (host booting) functions and definitions
 */
#include "headers.h"
#include "download.h"
#include "firmware.h"

#include <linux/vmalloc.h>

struct image_data g_wimax_image;

int load_wimax_image(int mode)
{
	struct file	*fp;
	int		read_size = 0;

	if (mode == AUTH_MODE)
		fp = klib_fopen(WIMAX_LOADER_PATH, O_RDONLY, 0);	/* download mode */
	else
		fp = klib_fopen(WIMAX_IMAGE_PATH, O_RDONLY, 0);		/* wimax mode */

	if (fp) {
		if (g_wimax_image.data == NULL)	{	/* check already allocated */
			g_wimax_image.data = (u_char *)vmalloc(MAX_WIMAXFW_SIZE);

			if (!g_wimax_image.data) {
				dump_debug("Error: Memory alloc failure");
				klib_fclose(fp);
				return STATUS_UNSUCCESSFUL;
			}
		}

		memset(g_wimax_image.data, 0, MAX_WIMAXFW_SIZE);
		read_size = klib_flen_fcopy(g_wimax_image.data, MAX_WIMAXFW_SIZE, fp);

		g_wimax_image.size = read_size;
		g_wimax_image.address = CMC732_WIMAX_ADDRESS;
		g_wimax_image.offset = 0;

		klib_fclose(fp);
	} else {
		dump_debug("Error: WiMAX image file open failed");
		return STATUS_UNSUCCESSFUL;
	}

	return STATUS_SUCCESS;
}

void unload_wimax_image(void)
{
	if (g_wimax_image.data != NULL) {
		dump_debug("Delete the Image Loaded");
		vfree(g_wimax_image.data);
		g_wimax_image.data = NULL;
	}
}

u_char send_cmd_packet(struct net_adapter *adapter, u_short cmd_id)
{
	struct hw_packet_header	*pkt_hdr;
	struct wimax_msg_header	*msg_hdr;
	u_char			tx_buf[CMD_MSG_TOTAL_LENGTH];
	int			status = 0;
	u_int			offset;
	u_int			size;

	pkt_hdr = (struct hw_packet_header *)tx_buf;
	pkt_hdr->id0 = 'W';
	pkt_hdr->id1 = 'C';
	pkt_hdr->length = be16_to_cpu(CMD_MSG_TOTAL_LENGTH);

	offset = sizeof(struct hw_packet_header);
	msg_hdr = (struct wimax_msg_header *)(tx_buf + offset);
	msg_hdr->type = be16_to_cpu(ETHERTYPE_DL);
	msg_hdr->id = be16_to_cpu(cmd_id);
	msg_hdr->length = be32_to_cpu(CMD_MSG_LENGTH);

	size = CMD_MSG_TOTAL_LENGTH;

	status = sd_send(adapter, tx_buf, size);
	if (status != STATUS_SUCCESS) {
		/* crc error or data error - set PCWRT '1' & send current type A packet again */
		dump_debug("hwSdioWrite : crc or data error");
		return status;
	}
	return status;
}

u_char send_image_info_packet(struct net_adapter *adapter, u_short cmd_id)
{
	struct hw_packet_header	*pkt_hdr;
	struct wimax_msg_header	*msg_hdr;
	u_int			image_info[4];
	u_char			tx_buf[IMAGE_INFO_MSG_TOTAL_LENGTH];
	int			status;
	u_int			offset;
	u_int			size;

	pkt_hdr = (struct hw_packet_header *)tx_buf;
	pkt_hdr->id0 = 'W';
	pkt_hdr->id1 = 'C';
	pkt_hdr->length = be16_to_cpu(IMAGE_INFO_MSG_TOTAL_LENGTH);

	offset = sizeof(struct hw_packet_header);
	msg_hdr = (struct wimax_msg_header *)(tx_buf + offset);
	msg_hdr->type = be16_to_cpu(ETHERTYPE_DL);
	msg_hdr->id = be16_to_cpu(cmd_id);
	msg_hdr->length = be32_to_cpu(IMAGE_INFO_MSG_LENGTH);

	image_info[0] = 0;
	image_info[1] = be32_to_cpu(g_wimax_image.size);
	image_info[2] = be32_to_cpu(g_wimax_image.address);
	image_info[3] = 0;

	offset += sizeof(struct wimax_msg_header);
	memcpy(&(tx_buf[offset]), image_info, sizeof(image_info));

	size = IMAGE_INFO_MSG_TOTAL_LENGTH;
	status = sd_send(adapter, tx_buf, size);

	if (status != STATUS_SUCCESS) {
		/* crc error or data error - set PCWRT '1' & send current type A packet again */
		dump_debug("hwSdioWrite : crc error");
		return status;
	}
	return status;
}

u_char send_image_data_packet(struct net_adapter *adapter, u_short cmd_id)
{
	struct hw_packet_header		*pkt_hdr;
	struct image_data_payload	*pImageDataPayload;
	struct wimax_msg_header		*msg_hdr;
	u_char				*tx_buf = NULL;
	int				status;
	u_int				len;
	u_int				offset;
	u_int				size;

	tx_buf = (u_char *)kmalloc(MAX_IMAGE_DATA_MSG_LENGTH, GFP_KERNEL);
	if (tx_buf == NULL) {
		dump_debug("MALLOC FAIL!!");
		return -1;
	}

	pkt_hdr = (struct hw_packet_header *)tx_buf;
	pkt_hdr->id0 = 'W';
	pkt_hdr->id1 = 'C';

	offset = sizeof(struct hw_packet_header);
	msg_hdr = (struct wimax_msg_header *)(tx_buf + offset);
	msg_hdr->type = be16_to_cpu(ETHERTYPE_DL);
	msg_hdr->id = be16_to_cpu(cmd_id);

	if (g_wimax_image.offset < (g_wimax_image.size - MAX_IMAGE_DATA_LENGTH))
		len = MAX_IMAGE_DATA_LENGTH;
	else
		len = g_wimax_image.size - g_wimax_image.offset;

	offset += sizeof(struct wimax_msg_header);
	pImageDataPayload = (struct image_data_payload *)(tx_buf + offset);
	pImageDataPayload->offset = be32_to_cpu(g_wimax_image.offset);
	pImageDataPayload->size = be32_to_cpu(len);

	memcpy(pImageDataPayload->data, g_wimax_image.data + g_wimax_image.offset, len);

	size = len + 8; /* length of Payload offset + length + data */
	pkt_hdr->length = be16_to_cpu(CMD_MSG_TOTAL_LENGTH + size);
	msg_hdr->length = be32_to_cpu(size);

	size = CMD_MSG_TOTAL_LENGTH + size;

	status = sd_send(adapter, tx_buf, size);

	if (status != STATUS_SUCCESS) {
		/* crc error or data error - set PCWRT '1' & send current type A packet again */
		dump_debug("hwSdioWrite : crc error");
		kfree(tx_buf);
		return status;
	}

	g_wimax_image.offset += len;

	kfree(tx_buf);

	return status;
}

/* used only during firmware download */
u_int sd_send(struct net_adapter *adapter, u_char *buffer, u_int len)
{
	int	nRet = 0;

	int nWriteIdx;

	len += (len & 1) ? 1 : 0;

	if (adapter->halted || adapter->removed) {
		dump_debug("Halted Already");
		return STATUS_UNSUCCESSFUL;
	}

	sdio_claim_host(adapter->func);
	hwSdioWriteBankIndex(adapter, &nWriteIdx, &nRet);

	if(nRet || (nWriteIdx < 0) )
		return STATUS_UNSUCCESSFUL;

	sdio_writeb(adapter->func, (nWriteIdx + 1) % 15, SDIO_H2C_WP_REG, NULL);

	nRet = sdio_memcpy_toio(adapter->func, SDIO_TX_BANK_ADDR+(SDIO_BANK_SIZE * nWriteIdx)+4, buffer, len);

	if (nRet < 0) {
		dump_debug("sd_send : error in sending packet!! nRet = %d",nRet);
	}

	nRet = sdio_memcpy_toio(adapter->func, SDIO_TX_BANK_ADDR + (SDIO_BANK_SIZE * nWriteIdx), &len, 4);

	if (nRet < 0) {
		dump_debug("sd_send : error in writing bank length!! nRet = %d",nRet);
	}
	sdio_release_host(adapter->func);

	return nRet;
}
