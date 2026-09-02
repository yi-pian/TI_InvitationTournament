"""Small layout-only adjustment after the requested P2-card compression."""
from docx import Document
from docx.shared import Pt
from docx.oxml.ns import qn

PATH = '电子设计竞赛单片机_信号题_完整Debug手册.docx'

doc = Document(PATH)
style = doc.styles['Normal']
style.font.name = 'Microsoft YaHei'
style._element.rPr.rFonts.set(qn('w:eastAsia'), 'Microsoft YaHei')
style.font.size = Pt(11.0)
style.paragraph_format.line_spacing = 1.23
style.paragraph_format.space_after = Pt(4)
doc.save(PATH)
print('adjusted', PATH)   
