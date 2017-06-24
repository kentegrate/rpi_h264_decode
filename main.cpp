// Video decode , resize and encode to jpg
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include “bcm_host.h”
#include “ilclient.h”
static  COMPONENT_T *image_encode = NULL;
static  char* filename  ;
int width,height;
void  donecallback(void *data, COMPONENT_T *comp)
{
  OMX_ERRORTYPE result;
  FILE *outf;
  OMX_BUFFERHEADERTYPE *out;
  out = ilclient_get_output_buffer(image_encode, 341, 1);
  if(out->nFilledLen == 0)
    {
      printf(“\nSOmthing is wrong in encoder”); fflush(stdout);
      return ;
    }
  outf = fopen(filename, “w”);
  if (outf == NULL)
    {
      printf(“Failed to open ‘%s’ for writing image\n”, filename);
      return ;
    }
  if (out != NULL)
    {
      printf(“%d bytes are ready to be put in JPG file “,out->nFilledLen);fflush(stdout);
      result = fwrite(out->pBuffer, 1, out->nFilledLen, outf);
      if (result != out->nFilledLen)
	{
	  printf(” \n%x Error emptying buffer to file”, result);
	}
      else
	{
	  printf(“Writing frame  %d bytes\n”,out->nFilledLen);
	}
      out->nFilledLen = 0;
    }
  else
    {
      printf(” \n No Buffer on  Image Encoding Output Port”);
    }
  fclose(outf);
  return ;
}
static int image_encoder(char *outputfilename ,OMX_U8* data, int len)
{
  OMX_IMAGE_PARAM_PORTFORMATTYPE format;
  OMX_PARAM_PORTDEFINITIONTYPE def;
  COMPONENT_T *list[5];
  OMX_BUFFERHEADERTYPE *buf;
  OMX_BUFFERHEADERTYPE *out;
  OMX_ERRORTYPE result;
  ILCLIENT_T *client;
  int status = 0;
  filename = outputfilename;
  bcm_host_init();
  memset(list, 0, sizeof(list));
  if ((client = ilclient_init()) == NULL) {
    return -3;
  }
  if (OMX_Init() != OMX_ErrorNone) {
    ilclient_destroy(client);
    return -4;
  }
  ilclient_set_fill_buffer_done_callback(client,donecallback ,NULL);
  // create image_encode
  result = ilclient_create_component(client, &image_encode, “image_encode”,
				      ILCLIENT_DISABLE_ALL_PORTS |
				      ILCLIENT_ENABLE_INPUT_BUFFERS |
				     ILCLIENT_ENABLE_OUTPUT_BUFFERS);
  if (result != 0)
    {
      printf(“ilclient_create_component() for image_encode failed with %x!\n”,result);
      return -1;
    }
  list[0] = image_encode;
  // get current settings of image_encode component from port 340
  memset(&def, 0, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
  def.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
  def.nVersion.nVersion = OMX_VERSION;
  def.nPortIndex = 340;
  if (OMX_GetParameter(ILC_GET_HANDLE(image_encode), OMX_IndexParamPortDefinition,&def) != OMX_ErrorNone)
    {
      printf(“%s:%d: OMX_GetParameter() for image_encode port 340 failed!\n”, __FUNCTION__, __LINE__);
      exit(1);
    }
  def.format.image.nFrameWidth = width;
  def.format.image.nFrameHeight = height;
  def.format.image.eCompressionFormat = OMX_COLOR_FormatUnused;
  def.format.image.nSliceHeight = def.format.image.nFrameHeight;
  def.format.image.nStride = def.format.image.nFrameWidth;
  def.format.image.eColorFormat = OMX_COLOR_Format32bitARGB8888;
  result = OMX_SetParameter(ILC_GET_HANDLE(image_encode),
			    OMX_IndexParamPortDefinition, &def);
  if (result != OMX_ErrorNone)
    {
      printf(“%s:%d: OMX_SetParameter() for image_encode port 340 failed with %x!\n”,__FUNCTION__, __LINE__, result);
      exit(1);
    }
  memset(&format, 0, sizeof(OMX_IMAGE_PARAM_PORTFORMATTYPE));
  format.nSize = sizeof(OMX_IMAGE_PARAM_PORTFORMATTYPE);
  format.nVersion.nVersion = OMX_VERSION;
  format.nPortIndex = 341;
  format.eCompressionFormat = OMX_IMAGE_CodingJPEG;
  printf(“OMX_SetParameter for image_encode:341…\n”);
  result = OMX_SetParameter(ILC_GET_HANDLE(image_encode),
			    OMX_IndexParamImagePortFormat, &format);
  if (result != OMX_ErrorNone)
    {
      printf(“%s:%d: OMX_SetParameter() for image_encode port 341 failed with %x!\n”,__FUNCTION__, __LINE__, result);
      exit(1);
    }
  printf(“encode to idle…\n”);
  if (ilclient_change_component_state(image_encode, OMX_StateIdle) == -1)
    {
      printf(“%s:%d: ilclient_change_component_state(image_encode, OMX_StateIdle) failed”,__FUNCTION__, __LINE__);
    }
  printf(“enabling port buffers for 340…\n”);
  if (ilclient_enable_port_buffers(image_encode, 340, NULL, NULL, NULL) != 0)
    {
      printf(“enabling port buffers for 340 failed!\n”);
      exit(1);
    }
  printf(“enabling port buffers for 341…\n”);
  if (ilclient_enable_port_buffers(image_encode, 341, NULL, NULL, NULL) != 0)
    {
      printf(“enabling port buffers for 341 failed!\n”);
      exit(1);
    }
  printf(“encode to executing…\n”);
  ilclient_change_component_state(image_encode, OMX_StateExecuting);
  printf(“looping for buffers…\n”);
  do {
    out = ilclient_get_output_buffer(image_encode, 341, 1);
    result = OMX_FillThisBuffer(ILC_GET_HANDLE(image_encode), out);
    if (result != OMX_ErrorNone)
      {
	printf(“Error filling buffer: %x\n”, result);
      }
    buf = ilclient_get_input_buffer(image_encode, 340, 1);
    if (buf == NULL)
      {
	printf(“Doh, no buffers for me!\n”);
      }
    else
      {
	printf(“filling %d bytes\n”,len);
	memcpy(buf->pBuffer, data ,len);
	buf->nFilledLen = len;
	if (OMX_EmptyThisBuffer(ILC_GET_HANDLE(image_encode), buf) !=
	    OMX_ErrorNone) {
	  printf(“Error emptying buffer!\n”);
	}
      }
  }
  while (0);
  //EOS management
  buf->nFilledLen = 0;
  buf->nFlags =  OMX_BUFFERFLAG_EOS;
  //OMX_FillThisBuffer(ILC_GET_HANDLE(image_encode), out);
  OMX_EmptyThisBuffer(ILC_GET_HANDLE(image_encode), buf);
  // wait for EOS from encoder
  ilclient_wait_for_event(image_encode, OMX_EventBufferFlag, 341, 0, OMX_BUFFERFLAG_EOS, 0,
			  ILCLIENT_BUFFER_FLAG_EOS, 10000);
  printf(“disabling port buffers for 340 and 341…\n”);
  //ilclient_disable_port_buffers(image_encode, 340, NULL, NULL, NULL);
  //ilclient_disable_port_buffers(image_encode, 341, NULL, NULL, NULL);
  //ilclient_state_transition(list, OMX_StateIdle);
  //ilclient_state_transition(list, OMX_StateLoaded);
  printf(“\n Encoding Complete , check JPG at input path”);fflush(stdout);
  ilclient_cleanup_components(list);
  OMX_Deinit();
  ilclient_destroy(client);
  return status;
}
static int video_decode_resize_encode(char *filename ,char* outputfilename ,int resize_width, int resize_height)
{
  OMX_VIDEO_PARAM_PORTFORMATTYPE format;
  COMPONENT_T *video_decode = NULL, *resize = NULL;
  COMPONENT_T *list[3];
  TUNNEL_T tunnel[2];
  ILCLIENT_T *client;
  int eos_flag =0;
  FILE *in;
  int status = 0;
  unsigned int data_length = 0;
  int packet_size = 80<<10;
  int resize_port_active =0;
  int frame_number = 0;
  width = resize_width;
  height = resize_height;
  memset(list, 0, sizeof(list));
  memset(tunnel, 0, sizeof(tunnel));
  if((in = fopen(filename, “rb”)) == NULL)
    return -1;
  if((client = ilclient_init()) == NULL)
    {
      fclose(in);
      return -2;
    }
  if(OMX_Init() != OMX_ErrorNone)
    {
      ilclient_destroy(client);
      fclose(in);
      return -3;
    }
  // create video_decode
  if(ilclient_create_component(client, &video_decode, “video_decode”, ILCLIENT_DISABLE_ALL_PORTS | ILCLIENT_ENABLE_INPUT_BUFFERS) != 0)
    status = -4;
  list[0] = video_decode;
  // create resize
  if(status == 0 && ilclient_create_component(client, &resize, “resize”, ILCLIENT_DISABLE_ALL_PORTS|ILCLIENT_ENABLE_OUTPUT_BUFFERS) != 0)
    status = -5;
  list[1] = resize;
  set_tunnel(tunnel, video_decode, 131, resize, 60);;
  if(status == 0)
    ilclient_change_component_state(video_decode, OMX_StateIdle);
  memset(&format, 0, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
  format.nSize = sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE);
  format.nVersion.nVersion = OMX_VERSION;
  format.nPortIndex = 130;
  format.eCompressionFormat = OMX_VIDEO_CodingAVC;
  if(status == 0 &&
     OMX_SetParameter(ILC_GET_HANDLE(video_decode), OMX_IndexParamVideoPortFormat, &format) == OMX_ErrorNone &&
     ilclient_enable_port_buffers(video_decode, 130, NULL, NULL, NULL) == 0)
    {
      OMX_BUFFERHEADERTYPE *buf , *outbuf;
      int port_settings_changed = 0;
      int first_packet = 1;
      ilclient_change_component_state(video_decode, OMX_StateExecuting);
      while((buf = ilclient_get_input_buffer(video_decode, 130, 1)) != NULL)
	{
	  // feed data and wait until we get port settings changed
	  unsigned char *dest = buf->pBuffer;
	  OMX_PARAM_PORTDEFINITIONTYPE portdef;
	  data_length += fread(dest, 1, packet_size-data_length, in);
	  printf(“\n Reading %d bytes from  file ” , data_length);fflush(stdout);
	  if(port_settings_changed == 0 &&
	     ((data_length > 0 && ilclient_remove_event(video_decode, OMX_EventPortSettingsChanged, 131, 0, 0, 1) == 0) ||
	      (data_length == 0 && ilclient_wait_for_event(video_decode, OMX_EventPortSettingsChanged, 131, 0, 0, 1,ILCLIENT_EVENT_ERROR | ILCLIENT_PARAMETER_CHANGED, 10000) == 0)))
	    {
	      port_settings_changed = 1;
	      // need to setup the input for the resizer with the output of the decoder
	      portdef.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
	      portdef.nVersion.nVersion = OMX_VERSION;
	      portdef.nPortIndex = 131;
	      OMX_GetParameter(ILC_GET_HANDLE(video_decode),OMX_IndexParamPortDefinition, &portdef);
	      // tell resizer input what the decoder output will be providing
	      portdef.nPortIndex = 60;
	      OMX_SetParameter(ILC_GET_HANDLE(resize),OMX_IndexParamPortDefinition, &portdef);
	      // need to setup the input for the resizer with the output of the decoder
	      portdef.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
	      portdef.nVersion.nVersion = OMX_VERSION;
	      portdef.nPortIndex = 61;
	      OMX_GetParameter(ILC_GET_HANDLE(resize),OMX_IndexParamPortDefinition, &portdef);
	      // change output color format and dimensions to match input
	      portdef.format.image.eCompressionFormat = OMX_IMAGE_CodingUnused;
	      portdef.format.image.eColorFormat = OMX_COLOR_Format32bitARGB8888;
	      portdef.format.image.nFrameWidth = width;
	      portdef.format.image.nFrameHeight = height;
	      portdef.format.image.nStride = 0;
	      portdef.format.image.nSliceHeight = 0;
	      portdef.format.image.bFlagErrorConcealment = OMX_FALSE;
	      OMX_SetParameter(ILC_GET_HANDLE(resize),OMX_IndexParamPortDefinition, &portdef);
	      // establish tunnel between video output and resizer input
	      if(ilclient_setup_tunnel(tunnel, 0, 0) != 0)
		{
		  status = -12;
		  break;
		}
	      // enable output of scheduler and input of resizer (ie enable tunnel)
	      OMX_SendCommand(ILC_GET_HANDLE(video_decode), OMX_CommandPortEnable, 131, NULL);
	      OMX_SendCommand(ILC_GET_HANDLE(resize),OMX_CommandPortEnable, 60, NULL);
	      // put resizer in idle state (this allows the outport of the decoder to become enabled)
	      ilclient_change_component_state(resize, OMX_StateExecuting);
	      // once the state changes, both ports should become enabled and the resizer output should generate a settings changed event
	      if( (ilclient_remove_event(resize, OMX_EventPortSettingsChanged, 61, 0, 0, 1) == 0))
		{
		  printf(“\n Enabling Resizer output “);
		  portdef.nPortIndex = 61;
		  OMX_GetParameter(ILC_GET_HANDLE(resize),OMX_IndexParamPortDefinition, &portdef);
		  if (ilclient_enable_port_buffers(resize, 61, NULL, NULL, NULL) != 0)
		    {
		      status = -13;
		      break;
		    }
		  ilclient_change_component_state(resize, OMX_StateExecuting);
		  resize_port_active =  1;
		}
	    }
	  //nothing left to read
	  if(!data_length)
	    {
	      //add eos if already not added
	      if (!eos_flag )
		{
		  printf(“\n adding EOS on input”);fflush(stdout);
		  buf->nFilledLen = 0;
		  buf->nFlags = OMX_BUFFERFLAG_TIME_UNKNOWN | OMX_BUFFERFLAG_EOS;
		  if(OMX_EmptyThisBuffer(ILC_GET_HANDLE(video_decode), buf) != OMX_ErrorNone)
		    status = -20;
		  eos_flag = 1;
		}
	    }
	  //output is ready
	  if(resize_port_active)
	    {
	      outbuf = NULL;
	      outbuf = ilclient_get_output_buffer(resize, 61, 1);
	      if(outbuf!=NULL && (outbuf->nFilledLen != 0))
		{
		  portdef.nPortIndex = 61;
		  OMX_GetParameter(ILC_GET_HANDLE(resize),OMX_IndexParamPortDefinition, &portdef);
		  frame_number ++;
		  //need to get fps from decode data to write logic,just to check show first and then 1 fps
		  if((frame_number == 1)||(frame_number %25 == 0))
		    {
		      printf(“\n need to convert %d bytes RGB to JPG  \n”,outbuf->nFilledLen);fflush(stdout);
		      image_encoder(outputfilename,outbuf->pBuffer,outbuf->nFilledLen);
		    }
		  else
		    {
		      printf(“\n need to skip this”);fflush(stdout);
		    }
		  //anyhow free the memory
		  OMX_FreeBuffer(ILC_GET_HANDLE(resize),61,outbuf);
		  outbuf->nFilledLen = 0;
		}
	      printf(“\n filling buffer with %d bytes”,outbuf->nFilledLen);fflush(stdout);
	      OMX_FillThisBuffer(ILC_GET_HANDLE(resize), outbuf);
	      printf(“\n  buffer filled”);fflush(stdout);
	    }
	  //resizer sees EOS in the stream on its output port ie work is done for this stream
	  if( (ilclient_remove_event(resize, OMX_EventBufferFlag, 61, 0, OMX_BUFFERFLAG_EOS,0)==0))
	    {
	      //  flush to allow video_decode to disable its input port
	      printf(“\n  received EOS on resizer output\n”);fflush(stdout);
	      break;
	    }
	  buf->nFilledLen = data_length;
	  data_length = 0;
	  buf->nOffset = 0;
	  if(first_packet)
	    {
	      buf->nFlags = OMX_BUFFERFLAG_STARTTIME;
	      first_packet = 0;
	    }
	  else
	    buf->nFlags = OMX_BUFFERFLAG_TIME_UNKNOWN;
	  printf(“\n emptying buffer %d bytes”,buf->nFilledLen);fflush(stdout);
	  if(OMX_EmptyThisBuffer(ILC_GET_HANDLE(video_decode), buf) != OMX_ErrorNone)
	    {
	      status = -14;
	      break;
	    }
	  printf(“\n  buffer emptied”);fflush(stdout);
	}
    }
  fclose(in);
  ilclient_flush_tunnels(tunnel, 0);
  //ilclient_disable_port_buffers(video_decode, 130, NULL, NULL, NULL);
  ilclient_disable_tunnel(tunnel);
  ilclient_teardown_tunnels(tunnel);
  ilclient_state_transition(list, OMX_StateIdle);
  ilclient_state_transition(list, OMX_StateLoaded);
  ilclient_cleanup_components(list);
  OMX_Deinit();
  ilclient_destroy(client);
  return status;
}
int main (int argc, char **argv)
{
  bcm_host_init();
  return video_decode_resize_encode(argv[1],argv[2],argv[3],argv[4]);
}
