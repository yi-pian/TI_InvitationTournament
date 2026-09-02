from docx import Document
from docx.oxml import OxmlElement
from docx.oxml.ns import qn

path = '电子设计竞赛单片机_信号题_比赛现场问题与解决方法大全.docx'
doc = Document(path)
for table in doc.tables:
    tr_pr = table.rows[0]._tr.get_or_add_trPr()
    header = OxmlElement('w:tblHeader')
    header.set(qn('w:val'), 'true')
    tr_pr.append(header)
doc.save(path)
