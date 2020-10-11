bool igrabber::save_subprocbuf(std::string filename, int side, int tab)
{
	bool bsaveBMP = true;
	std::size_t found = filename.find(".jpg");
	if (std::string::npos != found)
	{
		bsaveBMP = false;
	}

	MIL_ID system = get_system_id();
#ifndef _CRACK_
	MIL_ID pImageBuf = get_subprocbuf_id(side, tab);// m_impl->m_MilSubProcBuf[side][tab];
#else
	MIL_ID pImageBuf = get_cr_scanbuf_id(side, tab);
#endif

	MIL_ID pChildBuf = M_NULL;

	if (M_NULL == pImageBuf)
	{
		sprintf_s(m_log, _T("save_subprocbuf s:t[%d: %d] has error: image MID is null."), side, tab);
		AfxPrintInfo(m_log);
		return false;
	}

	_bstr_t bstr_file(filename.c_str());

	if (!bsaveBMP)
	{
		CSize szImage = m_sinfo.m_scan_info.get_pxl_subprocbuf(side, tab);

		if (szImage.cx < 1 || szImage.cy < 1)
		{
			sprintf_s(m_log, "save_subprocbuf has error: image size is under limit.");
			return false;
		}

		if (szImage.cx > JPG_MAX_WIDTH)
		{
			int di = (szImage.cx / (JPG_MAX_WIDTH+1)); /* 몇개로 나눌 수 있나 */
			int child_width = (szImage.cx / (di + 1)); /* 동일 사이즈로 분할 해서 */
			for (int i = 0; i <= di; i++)
			{
				MbufAlloc2d(system, child_width, szImage.cy, M_DEF_IMAGE_TYPE, M_IMAGE, &pChildBuf);
				MbufChild2d(pImageBuf, child_width*(i), 0, child_width, szImage.cy, &pChildBuf);

				std::string number = "_" + std::to_string(i);

				std::string filepathname = filename.substr(0, found) + number + ".jpg";

				//filename.insert(found, number);

				_bstr_t fname(filepathname.c_str());
				/*MbufSave(fname, pChildBuf); >> */MbufExport(fname, M_JPEG_LOSSY, pChildBuf);
				MbufFree(pChildBuf);
			}
		}
		else
		{
			MbufExport(bstr_file, M_JPEG_LOSSY, pImageBuf);
		}
	}

	else {
		MbufExport(bstr_file, M_BMP, pImageBuf);
	}

	return true;
}

/* image buffer의 내용을 bmp, jpg로 변환 저장한다. */
/* image size가 65536을 넘어가면 n등분해서 _1,2,3으로 jpg를 저장한다. */
bool igrabber::save_imagebuffer(std::string filename, int side, int tab)
{
	bool bsaveBMP = true;
	std::size_t found = filename.find(".jpg");
	if (std::string::npos != found)
	{
		bsaveBMP = false;
	}

	if (side < 0 || side >= index_max_side)
	{
		return false;
	}

	MIL_ID system = get_system_id();
#ifndef _CRACK_
	MIL_ID pImageBuf = get_procbuf_id(side, tab);
#else
	MIL_ID pImageBuf = get_cr_scanbuf_id(side, tab);
#endif
	MIL_ID pChildBuf = M_NULL;

	if (M_NULL == pImageBuf)
	{
		sprintf_s(m_log, "save_imagebuffer has error: image buffer is null.");
		AfxPrintInfo(m_log);
		return false;
	}


	_bstr_t bstr_file(filename.c_str());

	if (!bsaveBMP)
	{
		CSize szImage = m_sinfo.m_scan_info.get_pxl_procbuf(side, tab);

		if (szImage.cx < 1 || szImage.cy < 1)
		{
			sprintf_s(m_log, "save_imagebuffer has error: image size is under limit.");
			return false;
		}

		if (szImage.cx > JPG_MAX_WIDTH)
		{
			int di = (szImage.cx / (JPG_MAX_WIDTH+1)); /* 몇개로 나눌 수 있나 */
			int child_width = (szImage.cx / (di + 1)); /* 동일 사이즈로 분할 해서 */
			for (int i = 0; i <= di; i++)
			{
				MbufAlloc2d(system, child_width, szImage.cy, M_DEF_IMAGE_TYPE, M_IMAGE, &pChildBuf);
				MbufChild2d(pImageBuf, child_width*(i), 0, child_width, szImage.cy, &pChildBuf);

				std::string number = "_" + std::to_string(i);

				std::string filepathname = filename.substr(0, found) + number + ".jpg";

				//filename.insert(found, number);

				_bstr_t fname(filepathname.c_str());
				/*MbufSave(fname, pChildBuf); >> */MbufExport(fname, M_JPEG_LOSSY, pChildBuf);
				MbufFree(pChildBuf);
			}
		}
		else
		{
			MbufExport(bstr_file, M_JPEG_LOSSY, pImageBuf);
		}
	}

	else {
		MbufExport(bstr_file, M_BMP, pImageBuf);
	}

	return true;
}
